#include <Wire.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"

// Task handles
TaskHandle_t transmitTaskHandle;
TaskHandle_t receiveTaskHandle;

// Function prototypes
void transmitTask(void *pvParameters);
void receiveTask(void *pvParameters);
void commandTask(void *pvParameters);

volatile int pinStatus[10][5] = {};

StaticJsonDocument<512> sensorDataGlobal;

SemaphoreHandle_t mutex_d; //dataserialize

// CAN TWAI message to send
twai_message_t txMessage;

// Pins used to connect to CAN bus transceiver:
//#define CAN_RX 2 //using protoboard pins, RevA onwards://10
//#define CAN_TX 1 //using protoboard pins, RevA onwards://9
#define CAN_RX 10 //using protoboard pins, RevA onwards://10
#define CAN_TX 9 //using protoboard pins, RevA onwards://9
int canTXRXcount[2] = { 0, 0 };


// Create instances of TwoWire for different I2C interfaces
TwoWire I2C_one = TwoWire(0);
TwoWire I2C_two = TwoWire(1);

const int baudrate = 921600;
const int rows = 4;
const int cols = 50;
int sendCAN = 1;

//printing helper variables
int waitforADS = 0;
int printADS[4] = { 1, 1, 1, 1 };
int printCAN = 1;

String loopprint;

int cycledelay = 2;

// Define a queue to hold the JSON data
QueueHandle_t jsonQueue;

// Task for serializing and sending JSON data
void serializeTask(void *pvParameters) {
  StaticJsonDocument<512> sensorDataToSerialize;
  while (1) {
    // Wait for data to be placed into the queue
    if (xQueueReceive(jsonQueue, &sensorDataToSerialize, portMAX_DELAY) == pdTRUE) {
      // Serialize the JSON data and send it over Serial
      xSemaphoreTake(mutex_d, portMAX_DELAY); 
      size_t bytesWritten = serializeJson(sensorDataToSerialize, Serial);
      Serial.println();  // Add newline after the serialized data
      xSemaphoreGive(mutex_d); 
      //Serial.println(bytesWritten); 
    } else {
      yield();
      vTaskDelay(1);
    }
  }
}


void setup() {
  // Start the I2C interfaces
  Serial.begin(baudrate);  // Initialize Serial communication
  Serial.println("Begin");

  pinMode(CAN_TX, OUTPUT);
  pinMode(CAN_RX, INPUT);

  mutex_d = xSemaphoreCreateMutex(); 
  if (mutex_d == NULL) { 
  Serial.println("Mutex can not be created"); 
  } 


  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); //TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install and start the TWAI driver
  esp_err_t canStatus = twai_driver_install(&g_config, &t_config, &f_config);
  if (canStatus == ESP_OK) {
    Serial.println("CAN Driver installed");
  } else {
    Serial.println("CAN Driver installation failed");
  }
  twai_start();
  // Prepare the message to send
  txMessage.identifier = 0x01;           // Example identifier
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
  twai_transmit(&txMessage, pdMS_TO_TICKS(1));

  // Create transmit task
  //xTaskCreatePinnedToCore(transmitTask, "Transmit Task", 4096, NULL, 1, &transmitTaskHandle, 0);
  // Create receive task
  xTaskCreatePinnedToCore(receiveTask, "Receive Task", 2048, NULL, 2, &receiveTaskHandle, 1);
  xTaskCreatePinnedToCore(commandTask, "CommandTask", 2048, NULL, 1, NULL, 0);

  // Create the queue for holding JSON data (can hold 10 items)
  jsonQueue = xQueueCreate(6, sizeof(StaticJsonDocument<512>)); //3072

  // Create the serialize task
  xTaskCreatePinnedToCore(serializeTask, "Serialize Task", 4096, NULL, 2, NULL, 0);

  Serial.println("RTOS Tasks Created");
}


void transmitTask(void *pvParameters) {
  static int count;
  Serial.println("Start Transmit DAQ");
  while (1) {
    if (sendCAN == 1) {
      twai_transmit(&txMessage, pdMS_TO_TICKS(1));
      Serial.println("sent");
    }
    waitforADS = 0;
    Serial.print("Send Count: ");
    Serial.print(count++);
    Serial.println();
    // Serial.println(transmission_err == ESP_OK);
    // Serial.println(transmission_err == ESP_ERR_INVALID_ARG);
    // Serial.println(transmission_err == ESP_ERR_TIMEOUT);
    // Serial.println(transmission_err == ESP_FAIL);
    // Serial.println(transmission_err == ESP_ERR_INVALID_STATE);
    // Serial.println(transmission_err == ESP_ERR_NOT_SUPPORTED);
    delay(cycledelay);  // Delay for a second before reading again
  }
}

void command2pin(char solboardIDnum, char command, char mode){
  twai_message_t txMessage_command;
  int ID = solboardIDnum - '0';
  txMessage_command.identifier = ID*0x10 + 0x0F;           // Solenoid Board WRITE COMMAND 0xIDf
  txMessage_command.flags = TWAI_MSG_FLAG_EXTD;  // Example flags (extended frame)
  txMessage_command.data_length_code = 8;        // Example data length (8 bytes)
  txMessage_command.data[0] = 0xFF;              // Sol 0
  txMessage_command.data[1] = 0xFF;              // Sol 1
  txMessage_command.data[2] = 0xFF;              // Sol 2
  txMessage_command.data[3] = 0xFF;              // Sol 3
  txMessage_command.data[4] = 0xFF;              // Sol 4
  txMessage_command.data[5] = 0xFF;              // NIL
  txMessage_command.data[6] = 0xFF;              // NIL
  txMessage_command.data[7] = 0xFF;              // NIL

  int statusPin;
  if (command == '0') {statusPin = 0;}
  else if (command == '1') {statusPin = 1;}
  else if (command == '2') {statusPin = 2;}
  else if (command == '3') {statusPin = 3;}
  else if (command == '4') {statusPin = 4;}

  if (mode == '0') {
    pinStatus[ID][statusPin] = 0;
  } else if (mode == '1') {
    pinStatus[ID][statusPin] = 1;
  } else {
    Serial.println("Invalid mode");
  }

  for (int i = 0; i < 5; i++){
    txMessage_command.data[i] = pinStatus[ID][i];
    /*
    Serial.print("\t Pin ");
    Serial.print(i);
    Serial.print(":");
    Serial.print(pinStatus[i]);
    */
  }

  //Serial.println();

  twai_transmit(&txMessage_command, pdMS_TO_TICKS(1));
  //Serial.println("CAN sent");
    
}


void receiveTask(void *pvParameters) {
  StaticJsonDocument<512> sensorData;
  static String receivedCANmessagetoprint;
  Serial.println("Starting receive task...");
  delay(1000);

  while (1) {
    if (printCAN == 1) {
      twai_message_t rxMessage;
      esp_err_t receiveerror = twai_receive(&rxMessage, pdMS_TO_TICKS(50));

      if (receiveerror == ESP_OK) {
        // Copy rxMessage data at the start after ESP_OK
        twai_message_t copiedMessage;
        memcpy(&copiedMessage, &rxMessage, sizeof(twai_message_t));

        xSemaphoreTake(mutex_d, portMAX_DELAY); 

        // Prepare sensorData JSON
        sensorData["BoardID"] = String(copiedMessage.identifier, HEX);
        sensorData["SensorType"] = String((copiedMessage.identifier - 0x10 * (copiedMessage.identifier / 0x10)), HEX);

        for (int i = 0; i < copiedMessage.data_length_code; i++) {
          sensorData["Sensors"][i] = copiedMessage.data[i];
        }

        xSemaphoreGive(mutex_d); 
        // Add the JSON document to the queue for serialization
        if (xQueueSend(jsonQueue, &sensorData, portMAX_DELAY) != pdTRUE) {
          Serial.println("Queue is full, data not sent to serialization task");
        }
        
        // Print the received message
        canTXRXcount[1] += 1;
        receivedCANmessagetoprint += "Total Can Received: ";
        receivedCANmessagetoprint += String(canTXRXcount[1]) + "\n";
        receivedCANmessagetoprint += "Received Message - ID: 0x";
        receivedCANmessagetoprint += String(copiedMessage.identifier, HEX);
        receivedCANmessagetoprint += ", DLC: ";
        receivedCANmessagetoprint += String(copiedMessage.data_length_code);
        receivedCANmessagetoprint += ", Data: ";
        for (uint8_t i = 0; i < copiedMessage.data_length_code; i++) {
          receivedCANmessagetoprint += String(copiedMessage.data[i], HEX);
          receivedCANmessagetoprint += " ";
        }
        receivedCANmessagetoprint += "\n";

        // Serialize the sensorData JSON to Serial
        //serializeJson(sensorData, Serial);
        //Serial.println();
        receivedCANmessagetoprint = "";
      }
    }
  }
}


void commandTask(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    String message = Serial.readStringUntil('\n');
    char solboardIDnum = message[0];
    char command = message[1];
    char mode = message[2];
    switch (command) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
        command2pin(solboardIDnum,command,mode);
        break;
      default:
        //Serial.println("Invalid command");
        vTaskDelay(10 / portTICK_PERIOD_MS);
      yield();
      vTaskDelay(10);
    }
  }
}


void loop() {
  static String loopmessagetoprint;
  yield();
  delay(1000);
  //Serial.println("CAN Reader Active");
}
