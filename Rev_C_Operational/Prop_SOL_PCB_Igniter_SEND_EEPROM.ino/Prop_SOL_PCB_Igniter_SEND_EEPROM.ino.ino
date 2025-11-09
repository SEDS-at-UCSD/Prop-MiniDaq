#include <Wire.h>
#include <SPI.h>
#include <Adafruit_INA260.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include <EEPROM.h>

#define EEPROM_ADDRESS 0 // Address to store the board ID in EEPROM

// CAN TWAI message to send
twai_message_t txMessage;
// Pins used to connect to CAN bus transceiver:
#define CAN_RX 10
#define CAN_TX 9
int canTXRXcount[2] = {0,0};
volatile int baseID_int = 6;
volatile int baseID  = (0x10)*baseID_int;

// Function to read baseID_int from EEPROM
void readBaseIDFromEEPROM() {
  EEPROM.begin(512); // Initialize EEPROM with size 512 bytes
  baseID_int = EEPROM.read(EEPROM_ADDRESS); // Read stored baseID_int
  baseID = (0x10) * baseID_int; // Update baseID based on baseID_int
  Serial.print("Stored baseID_int: ");
  Serial.println(baseID_int);
  Serial.print("Computed baseID: ");
  Serial.println(baseID, HEX); // Print baseID in hex format
}

// Function to write baseID_int to EEPROM if it has changed
void writeBaseIDToEEPROM(int newBaseIDInt) {
  if (baseID_int != newBaseIDInt) {
    Serial.print("Writing new baseID_int to EEPROM: ");
    Serial.println(newBaseIDInt);
    EEPROM.write(EEPROM_ADDRESS, newBaseIDInt); // Write new baseID_int to EEPROM
    EEPROM.commit(); // Commit changes to EEPROM
    baseID_int = newBaseIDInt; // Update the volatile baseID_int
    baseID = (0x10) * baseID_int; // Recalculate baseID
    Serial.print("New computed baseID: ");
    Serial.println(baseID, HEX); // Print the updated baseID
  } else {
    Serial.println("baseID_int hasn't changed, skipping EEPROM write.");
  }
}


StaticJsonDocument<512> sensorData;
StaticJsonDocument<512> solenoidsData;

#define CHANNEL_0_PIN 11
#define CHANNEL_1_PIN 12
#define CHANNEL_2_PIN 13
#define CHANNEL_3_PIN 14
#define CHANNEL_4_PIN 35 //not in use
#define EXTERNAL_POWER_PIN 8

uint8_t pinChannels[5] = {CHANNEL_0_PIN,CHANNEL_1_PIN,CHANNEL_2_PIN,CHANNEL_3_PIN,CHANNEL_4_PIN};
uint32_t pinFreq[5] = {6,6,6,6,6}; //200Hz

volatile int pinStatus[5] = {0,0,0,0,0}; 
volatile uint32_t pinLevels[5] = {255,255,255,255,255}; //uint8, default HIGH


// Hardware Serial 2 pins
#define RXD2 19 // 19 expected
#define TXD2 18 // 18 expected

Adafruit_INA260 ina260;
QueueHandle_t powerQueue;

struct PowerData {
  float voltage;
  float current;
} data;

const char* deviceType = "Driver"; // Example device type

void powerTask(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    float voltage = ina260.readBusVoltage()/1000; // mV -> V
    float current = ina260.readCurrent()/1000; // mA -> A

    // Create a struct to hold power data
    struct PowerData {
      float voltage;
      float current;
    } data;

    data.voltage = voltage;
    data.current = current;

    // Send power data to the queue
    xQueueSend(powerQueue, &data, portMAX_DELAY);

    vTaskDelay(10 / portTICK_PERIOD_MS); // Adjust the delay as needed
  }
}


void solClockTask(void *pvParameters) {
  (void)pvParameters;
  static int pinPol[5] = {0, 0, 0, 0, 0};
  while (1) {
    for (int i = 0; i < 5; i++) {
      if (pinStatus[i] == 1) { // Assuming pinStatus and statusPin are defined elsewhere
        pinPol[i] = !pinPol[i]; // Toggle 0/1
        if (pinPol[i] == 0){
          digitalWrite(pinChannels[i], LOW); // Assuming channelPin is defined for each channel
        } else {
          digitalWrite(pinChannels[i], HIGH); // Assuming channelPin is defined for each channel
        }
        Serial.print(i);
        Serial.print("\t");
        Serial.print(pinChannels[i]);
        Serial.print("\t");
        Serial.println(pinPol[i]);
      }
    }
    
    vTaskDelay((1000/(2*pinFreq[0])) / portTICK_PERIOD_MS); // Delay 166 ms, considering FreeRTOS tick period
  }
}

void command2pin(char command, String mode){
  int channelPin, statusPin;
  if (command == '0') {channelPin = CHANNEL_0_PIN; statusPin = 0;}
  else if (command == '1') {channelPin = CHANNEL_1_PIN; statusPin = 1;}
  else if (command == '2') {channelPin = CHANNEL_2_PIN; statusPin = 2;}
  else if (command == '3') {channelPin = CHANNEL_3_PIN; statusPin = 3;}
  else if (command == '4') {channelPin = CHANNEL_4_PIN; statusPin = 4;}

  if (mode[0] == '0') {
    pinStatus[statusPin] = 0;
    //ledcWrite(pinChannels[statusPin], 0);
    //digitalWrite(channelPin, LOW);

    //Serial.print("Channel ");
    //Serial.print(command);
    //Serial.println(" turned OFF");
  } else if (mode[0] == '1') {
    pinStatus[statusPin] = 1;
    //ledcWrite(pinChannels[statusPin], pinLevels[statusPin]);
    //digitalWrite(channelPin, HIGH);

    //Serial.print(command);
    //Serial.println(" turned ON");
  } else if (mode[0] == 'f' || mode[0] == 'F') {
    pinLevels[statusPin] = (mode.substring(1)).toInt();
    //ledcWrite(pinChannels[statusPin], pinLevels[statusPin]);
  } else {
    Serial.println("Invalid mode");
  }

}

void receiveTask(void *pvParameters) {
  static String receivedCANmessagetoprint;
  while(1){
    twai_message_t rxMessage;
    esp_err_t receiveerror = twai_receive(&rxMessage, pdMS_TO_TICKS(50));
    if (receiveerror == ESP_OK) {
      // Copy rxMessage data at the start after ESP_OK
      twai_message_t copiedMessage;
      memcpy(&copiedMessage, &rxMessage, sizeof(twai_message_t));
      if (copiedMessage.identifier == (baseID + 0x0f)){ //Check if Write Command issued at 0x5f (Conversion always lower case)
        //Serial.println("RECEIVED CAN"); //for CAN Debug
        char commands[5] = {'0','1','2','3','4'};
        String modes[2] = {"0","1"};
        for (int i = 0; i < 5; i++){
          int mode = copiedMessage.data[i];
          command2pin(commands[i],String(mode));
          Serial.println("i = " + String(i) + "," + String(mode));
        }
      }
      else if (copiedMessage.identifier > 0x1F){ //for CAN Debug
        //Serial.print("Received from: ");
        //Serial.println(String(rxMessage.identifier, HEX));
      }
      


      /** //This is for generic printing
      //Print the received message
      canTXRXcount[1] += 1;
      receivedCANmessagetoprint += "Total Can Received: ";
      receivedCANmessagetoprint += String(canTXRXcount[1]) + "\n";
      receivedCANmessagetoprint += "Received Message - ID: 0x";
      receivedCANmessagetoprint += String(rxMessage.identifier, HEX);
      receivedCANmessagetoprint += ", DLC: ";
      receivedCANmessagetoprint += String(rxMessage.data_length_code);
      receivedCANmessagetoprint += ", Data: ";
      for (uint8_t i = 0; i < rxMessage.data_length_code; i++) {
        receivedCANmessagetoprint += String(rxMessage.data[i], HEX);
        receivedCANmessagetoprint += " ";
      }
      receivedCANmessagetoprint += "\n";
      Serial.println(receivedCANmessagetoprint);
      receivedCANmessagetoprint = "";
      **/
    }
  }
}

void commandTask(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    String readbuffer = Serial.readStringUntil('\n');
    char command = readbuffer[0];
    String mode = readbuffer.substring(1);
    //Serial.println(command);
    //Serial.println(mode);
    switch (command) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
        command2pin(command,mode);
        break;
      case 'e':
        pinMode(EXTERNAL_POWER_PIN, INPUT_PULLUP);
        Serial.print("Analog Ext Power Pin:");
        Serial.print(analogRead(EXTERNAL_POWER_PIN));
        if (digitalRead(EXTERNAL_POWER_PIN) == LOW) {
          Serial.println("\t External power is connected.");
        } else {
          Serial.println("\t External power is not connected.");
        }
        break;
      default:
        //Serial.println("Invalid command");
        vTaskDelay(10);
    }
  }
}

void sendJsonResponse(const char* action, const char* value) {
  StaticJsonDocument<200> doc;
  doc["action"] = action;
  doc["value"] = value;
  String message;
  serializeJson(doc, message);
  Serial1.println(message);
  //Serial.println("Sent: " + message); // For debugging
}

void sendCurrentCanId() {
  sendJsonResponse("CANID", String(baseID).c_str());
}

void sendDeviceType() {
  sendJsonResponse("DEVICE_TYPE", deviceType);
}

void processJsonMessage(String message) { //for Screen
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.println("deserializeJson() failed: ");
    Serial.println(error.c_str());
    command2pin(message[0],message.substring(1));
    Serial.println("Attempting Solenoid Command: " + message);
    Serial.println("Attempting Solenoid Ch: " + String(message[0]));
    Serial.println("Attempting Solenoid Mode: " + message.substring(1));
    return;
  }

  const char* action = doc["action"];
  const char* value =  doc["value"];
  int currID = baseID_int;
  if (strcmp(action, "CANID") == 0) {
    if (strcmp(value, "INC") == 0) {
      currID++;
    } else if (strcmp(value, "DEC") == 0) {
      currID--;
    } else if (strcmp(value, "?") == 0) {
      sendCurrentCanId();
    } else {
      int setID = 0;
      bool isValid = true;

      // Check if the value is a valid number
      for (int i = 0; value[i] != '\0'; i++) {
        if (!isDigit(value[i])) {
          isValid = false;
          break;
        }
      }

      if (isValid) {
        setID = atoi(value); // Convert to int
        // Additional check for successful conversion
        if (setID == 0 && strcmp(value, "0") != 0) {
          Serial.println("Conversion failed, value was not a valid number.");
        } else {
          currID = setID;
          Serial.print("Conversion successful, setID = ");
          Serial.println(setID);
        }
      } else {
        Serial.println("Invalid input: not a number.");
      }
    }
    writeBaseIDToEEPROM(currID);
    baseID_int = currID;
    baseID  = (0x10)*baseID_int;
  } else if (strcmp(action, "DEVICE_TYPE") == 0) {
    if (strcmp(value, "?") == 0) {
      sendDeviceType();
    } else {
      Serial.println("Unknown DEVICE_TYPE action: " + String(value));
    }
  } else {
    Serial.println("Unknown action: " + String(action));
  }
}

void serialTask(void * parameter) {
  while(true) {
    if (Serial1.available()) {
      String incomingMessage = Serial1.readStringUntil('\n');
      Serial.println("Received: " + incomingMessage); // For debugging
      processJsonMessage(incomingMessage);
    }
    // Add a small delay to prevent task from hogging CPU
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(921600);
  pinMode(RXD2, INPUT);
  pinMode(TXD2, OUTPUT);
  delay(10);
  Serial1.begin(921600,SERIAL_8N1, RXD2, TXD2);

  // Read the baseID_int from EEPROM at the start
  readBaseIDFromEEPROM();
  
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install and start the TWAI driver
  esp_err_t canStatus = twai_driver_install(&g_config, &t_config, &f_config);
  if (canStatus == ESP_OK) {
    Serial.println("CAN Driver installed");
  } else {
    Serial.println("CAN Driver installation failed");
  }
  twai_start();

  txMessage.identifier = baseID + 0x00;           // Solenoid Board identifier Board 0x5X, 0x50 boot id
  txMessage.flags = TWAI_MSG_FLAG_EXTD;  // Example flags (extended frame)
  txMessage.data_length_code = 8;        // Example data length (8 bytes)
  txMessage.data[0] = 0xFF;              // Reserved for message type
  txMessage.data[1] = 0xFF;              // Sensor 1: Temperature
  txMessage.data[2] = 0xFF;              // Sensor 2: Humidity
  txMessage.data[3] = 0xFF;              // Sensor 3: X axis Acceleration
  txMessage.data[4] = 0xFF;              // Sensor 4: Y axis Acceleration
  txMessage.data[5] = 0xFF;              // Sensor 5: Z axis Acceleration
  txMessage.data[6] = 0xFF;              // Sensor 6: Power Use in W
  txMessage.data[7] = 0xFF;              // Reserved for message Count

  Serial.println("CAN Started");

  Wire.begin(16, 17); // SDA on GPIO 16, SCL on GPIO 17
  pinMode(EXTERNAL_POWER_PIN, INPUT_PULLUP);
  pinMode(CHANNEL_0_PIN, OUTPUT);
  pinMode(CHANNEL_1_PIN, OUTPUT);
  pinMode(CHANNEL_2_PIN, OUTPUT);
  pinMode(CHANNEL_3_PIN, OUTPUT);
  pinMode(CHANNEL_4_PIN, OUTPUT);
  for (int i = 0; i < 5; i++){
    //ledcAttach(pinChannels[i], pinFreq[i], 8); //8 bit = 255 max
  }

  if (!ina260.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  
  ina260.setAveragingCount(INA260_COUNT_4);   // set the number of samples to average
  // set the time over which to measure the current and bus voltage
  ina260.setVoltageConversionTime(INA260_TIME_140_us);
  ina260.setCurrentConversionTime(INA260_TIME_140_us);
  ina260.setMode(INA260_MODE_CONTINUOUS);

  powerQueue = xQueueCreate(1, sizeof(struct PowerData));

  // Create and start the tasks
  xTaskCreatePinnedToCore(receiveTask, "Receive Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(powerTask, "PowerTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(commandTask, "CommandTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(serialTask, "SerialTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(solClockTask, "solClockTask", 4096, NULL, 1, NULL, 1);
}

void loop() { //print Solenoid status
  // Check if there's power data in the queue
  struct PowerData data;
  if (xQueueReceive(powerQueue, &data, 0) == pdTRUE) {
    txMessage.identifier = baseID + 0x01;

    sensorData["BoardID"] = "Board " + String(baseID_int);
    sensorData["SensorType"] = "Voltage";
    sensorData["Sensors"] = data.voltage;

    serializeJson(sensorData, Serial);
    Serial.println();

    sensorData["BoardID"] = "Board " + String(baseID_int);
    sensorData["SensorType"] = "Current";
    sensorData["Sensors"] = data.current;

    serializeJson(sensorData, Serial);
    Serial.println();

    solenoidsData["BoardID"] = "Board " + String(baseID_int);
    solenoidsData["SensorType"] = "Solenoids";
    for (int i = 0; i < 5; i++){
          solenoidsData["Sensors"][i] = pinStatus[i];
      }

    serializeJson(solenoidsData, Serial);
    Serial.println();

    /*
    Serial.print("Bus Voltage: ");
    Serial.print(data.voltage);
    Serial.print(" V, Current: ");
    Serial.print(data.current);
    Serial.println(" A");
    */
    for (int i = 0; i < 5; i++){
      txMessage.data[i] = pinStatus[i];
      /*
      Serial.print("\t Pin ");
      Serial.print(i);
      Serial.print(":");
      Serial.print(pinStatus[i]);
      Serial.println();
      */
    }
    esp_err_t canStatus = twai_transmit(&txMessage, pdMS_TO_TICKS(1));
    if (canStatus != ESP_OK){
      Serial.println(canStatus);
      if (canStatus == ESP_ERR_INVALID_STATE){
        Serial.println("INVALID STATE");
        //esp_restart(); // Trigger software reset
        twai_stop();
        twai_driver_uninstall();
        delay(10);
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); //TWAI_TIMING_CONFIG_500KBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
        twai_driver_install(&g_config, &t_config, &f_config);
        twai_start();
        delay(1);
      }
    }
    
    sendCurrentCanId();

    delay(5); //50 -> 20 hz

  }
  
}
