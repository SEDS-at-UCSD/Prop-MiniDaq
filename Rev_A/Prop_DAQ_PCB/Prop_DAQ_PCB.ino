#include <Wire.h>
#include <ADS1X15.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"

// Task handles
TaskHandle_t transmitTaskHandle;
TaskHandle_t receiveTaskHandle;

// Function prototypes
void transmitTask(void *pvParameters);
void receiveTask(void *pvParameters);

// CAN TWAI message to send
twai_message_t txMessage;

// Pins used to connect to CAN bus transceiver:
#define CAN_RX 10
#define CAN_TX 9
int canTXRXcount[2] = {0,0};

// Pins used to connect to Serial bus :
#define TX2 19
#define RX2 8


// Create instances of TwoWire for different I2C interfaces
TwoWire I2C_one = TwoWire(0);
TwoWire I2C_two = TwoWire(1);

// Define your I2C addresses
#define I2C_ADDR_1015_1 0x48
#define I2C_ADDR_1015_2 0x49
#define I2C_ADDR_1115_1 0x48
#define I2C_ADDR_1115_2 0x49

// Create instances of ADS1X15 for each device
ADS1015 ads_1015_1(I2C_ADDR_1015_1, &I2C_one);
ADS1015 ads_1015_2(I2C_ADDR_1015_2, &I2C_one);
ADS1115 ads_1115_1(I2C_ADDR_1115_1, &I2C_two);
ADS1115 ads_1115_2(I2C_ADDR_1115_2, &I2C_two);

const int baudrate = 921600;
const int rows = 4;
const int cols = 50;
int ADS1015_1_Data[rows][cols];
int ADS1015_2_Data[rows][cols];
int ADS1115_1_Data[rows][cols];
int ADS1115_2_Data[rows][cols];

int sendCAN = 1;

//printing helper variables
int waitforADS = 0;
int printADS[4] = {1,1,1,1};
int printCAN = 0;

String loopprint;

int cycledelay = 2;


void setup() {
  // Start the I2C interfaces
  I2C_one.begin(16,17);
  I2C_two.begin(42,41);
  Serial.begin(baudrate); // Initialize Serial communication
  Serial2.begin(baudrate,SERIAL_8N1,RX2,TX2);
  Serial.println("Begin");
  analogReadResolution(12);

  // Start the ADS1X15 devices
  ads_1015_1.begin();
  ads_1015_2.begin();
  ads_1115_1.begin();
  ads_1115_2.begin();

  ads_1015_1.setWireClock(400000);
  ads_1015_2.setWireClock(400000);
  ads_1115_1.setWireClock(400000);
  ads_1115_2.setWireClock(400000);

  ads_1015_1.setMode(1);
  ads_1015_2.setMode(1);
  ads_1115_1.setMode(1);
  ads_1115_2.setMode(1);

  ads_1015_1.setDataRate(7);
  ads_1015_2.setDataRate(7);
  ads_1115_1.setDataRate(7);
  ads_1115_2.setDataRate(7);
  Serial.println("ADS Devices Started");

  pinMode(CAN_TX,OUTPUT);
  pinMode(CAN_RX,INPUT);
  pinMode(46,OUTPUT_OPEN_DRAIN);
  digitalWrite(46,LOW);
  pinMode(47,INPUT_PULLUP);

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
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
  txMessage.identifier = 0x10;           // Example identifier
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

  // Create transmit task
  xTaskCreatePinnedToCore(transmitTask, "Transmit Task", 4096, NULL, 1, &transmitTaskHandle, 0);

  // Create receive task
  xTaskCreatePinnedToCore(receiveTask, "Receive Task", 2048, NULL, 1, &receiveTaskHandle, 1);

  Serial.println("RTOS Tasks Created");
}


void transmitTask(void *pvParameters) {
  static int count;
  Serial.println("Start Transmit DAQ");
  while (1) {
    //Internal ADC for reference 2.5V
    float analogVolts1 = (3.3/4095)*analogRead(1);
    float analogVolts2 = (3.3/4095)*analogRead(2);
    
    // print out the values you read:
    Serial.printf("ADC 2V5_0 value1 = %f\n",analogVolts1);
    Serial.printf("ADC 2V5_1 value2 = %f\n",analogVolts2);
    
    // Read and print values from ADS1015 devices
    ads_1015_1.setGain(0);
    ads_1015_2.setGain(0);
    ads_1115_1.setGain(0);
    ads_1115_2.setGain(0);
    waitforADS = 1;
    Serial.println("--- start ADS read ---");
    for (int channel = 0; channel < 4; ++channel) {
      float f = ads_1015_1.toVoltage(); 
      //int value = ads_1015_1.readADC(channel); //11 bit Mantissa, one bit for sign
      int value = 0; //
      if (channel == 0){
        value = ads_1015_1.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 1){
        value = ads_1015_1.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 2){
        value = ads_1015_1.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 3){
        value = ads_1015_1.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }

      for (int j = (cols-1); j > 0; --j) {
        ADS1015_1_Data[channel][j] = ADS1015_1_Data[channel][j-1]; //shift buffer, LSB is newest data
      }
      ADS1015_1_Data[channel][0] = value;

      int mean_value = 0;
      for (int j = 0; j < cols; ++j) {
        mean_value += ADS1015_1_Data[channel][j]; //get sum
      }
      mean_value /= cols;

      if (printADS[0] == 1){
        Serial.print("ADS1015_1 Ch");
        Serial.print(channel);
        Serial.print(": ");
        Serial.print(f*value,4);
        Serial.print("\t mean: ");
        Serial.println(f*mean_value,4);
      }
        txMessage.identifier = 0x11;           // Example identifier
        txMessage.flags = TWAI_MSG_FLAG_EXTD;  // Example flags (extended frame)
        txMessage.data_length_code = 8;        // Example data length (8 bytes)
        txMessage.data[2*channel] = value/256;              
        txMessage.data[2*channel+1] = value%256;              
    }

    if (sendCAN == 1){
      twai_transmit(&txMessage, pdMS_TO_TICKS(1));
    }

    for (int channel = 0; channel < 4; ++channel) {
      float f = ads_1015_2.toVoltage();

      // int value = ads_1015_2.readADC(channel); //11 bit Mantissa, one bit for sign
      
      int value = 0;
      if (channel == 0){
        value = ads_1015_2.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 1){
        value = ads_1015_2.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 2){
        value = ads_1015_2.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 3){
        value = ads_1015_2.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }

      for (int j = (cols-1); j > 0; --j) {
        ADS1015_2_Data[channel][j] = ADS1015_2_Data[channel][j-1]; //shift buffer, LSB is newest data
      }
      ADS1015_2_Data[channel][0] = value;

      int mean_value = 0;
      for (int j = 0; j < cols; ++j) {
        mean_value += ADS1015_2_Data[channel][j]; //get sum
      }
      mean_value /= cols;

      if (printADS[0] == 1){
        Serial.print("ADS1015_2 Ch");
        Serial.print(channel);
        Serial.print(": ");
        Serial.print(f*value,4);
        Serial.print("\t mean: ");
        Serial.println(f*mean_value,4);
      }
        txMessage.identifier = 0x12;           // Example identifier
        txMessage.flags = TWAI_MSG_FLAG_EXTD;  // Example flags (extended frame)
        txMessage.data_length_code = 8;        // Example data length (8 bytes)
        txMessage.data[2*channel] = value/256;              
        txMessage.data[2*channel+1] = value%256;              
    }
    if (sendCAN == 1){
      twai_transmit(&txMessage, pdMS_TO_TICKS(1));
    }
    // Read and print values from ADS1115 devices
    int v1,v2;
    float f_val;
    for (int channel = 0; channel < 4; ++channel) {
      float f = ads_1115_1.toVoltage();
      f_val = f;

      //      int value = ads_1115_1.readADC(channel); //11 bit Mantissa, one bit for sign
      
      /***int value = 0; //
      if (channel == 0){
        value = ads_1115_1.readADC_Differential_0_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 1){
        value = ads_1115_1.readADC_Differential_1_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 2){
        value = ads_1115_1.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 3){
        value = 0; //11 bit Mantissa, one bit for sign
      }
      value += (0.0/f);
      ***/

      int value = 0;
      if (channel == 0){
        value = ads_1115_1.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 1){
        value = ads_1115_1.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 2){
        value = ads_1115_1.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 3){
        value = ads_1115_1.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }


      for (int j = (cols-1); j > 0; --j) {
        ADS1115_1_Data[channel][j] = ADS1115_1_Data[channel][j-1]; //shift buffer, LSB is newest data
      }
      ADS1115_1_Data[channel][0] = value;

      int mean_value = 0;
      for (int j = 0; j < cols; ++j) {
        mean_value += ADS1115_1_Data[channel][j]; //get sum
      }
      mean_value /= cols;

      if (channel == 1){
        v1 = mean_value;
      }else if (channel == 3) {
        v2 = mean_value;
      }

      if (printADS[2] == 1){
        Serial.print("ADS1115_1 Ch");
        Serial.print(channel);
        Serial.print(": ");
        Serial.print(f*value,4);
        Serial.print("\t mean: ");
        Serial.println(f*mean_value,4);
      }
      txMessage.identifier = 0x13;           // Example identifier
      txMessage.flags = TWAI_MSG_FLAG_EXTD;  // Example flags (extended frame)
      txMessage.data_length_code = 8;        // Example data length (8 bytes)
      txMessage.data[2*channel] = value/256;              
      txMessage.data[2*channel+1] = value%256;              
    }
    Serial.println(v1);
    Serial.println(v2);
    float tempmillivolt = 1000.0*f_val*(v1);
    Serial.print("Voltdiff mV: ");
    Serial.println(tempmillivolt,4);

    float kfactor = (16.397-(-5.891))/(400-(-200)); //4.096 mV per 100 C
    float temp = tempmillivolt/kfactor;
    Serial.print("Kfactor: ");
    Serial.println(kfactor,4);


    Serial.print("Temp: ");
    Serial.println(temp);

    if (sendCAN == 1){
      twai_transmit(&txMessage, pdMS_TO_TICKS(1));
    }

    for (int channel = 0; channel < 4; ++channel) {
      float f = ads_1115_2.toVoltage();
      f_val = f;
      //int value = ads_1115_2.readADC(channel); //11 bit Mantissa, one bit for sign

      int value = 0;
      if (channel == 0){
        value = ads_1115_2.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 1){
        value = ads_1115_2.readADC_Differential_0_1(); //11 bit Mantissa, one bit for sign
      }else if (channel == 2){
        value = ads_1115_2.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }else if (channel == 3){
        value = ads_1115_2.readADC_Differential_2_3(); //11 bit Mantissa, one bit for sign
      }

      for (int j = (cols-1); j > 0; --j) {
        ADS1115_2_Data[channel][j] = ADS1115_2_Data[channel][j-1]; //shift buffer, LSB is newest data
      }
      ADS1115_2_Data[channel][0] = value;

      int mean_value = 0;
      for (int j = 0; j < cols; ++j) {
        mean_value += ADS1115_2_Data[channel][j]; //get sum
      }
      mean_value /= cols;

      if (channel == 1){
        v1 = mean_value;
      }else if (channel == 3) {
        v2 = mean_value;
      }

      if (printADS[2] == 1){
        Serial.print("ADS1115_2 Ch");
        Serial.print(channel);
        Serial.print(": ");
        Serial.print(f*value,4);
        Serial.print("\t mean: ");
        Serial.println(f*mean_value,4);
      }
      txMessage.identifier = 0x14;           // Example identifier
      txMessage.flags = TWAI_MSG_FLAG_EXTD;  // Example flags (extended frame)
      txMessage.data_length_code = 8;        // Example data length (8 bytes)
      txMessage.data[2*channel] = value/256;              
      txMessage.data[2*channel+1] = value%256;              
    }
    if (sendCAN == 1){
      twai_transmit(&txMessage, pdMS_TO_TICKS(1));
    }

    Serial.println("--- end ADS read ---");
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
    Serial2.println("Hello there!");
    delay(cycledelay); // Delay for a second before reading again
  }
}

void receiveTask(void *pvParameters) {
  static String receivedCANmessagetoprint;
  while(1){
    if (printCAN == 1){
      twai_message_t rxMessage;
      esp_err_t receiveerror = twai_receive(&rxMessage, pdMS_TO_TICKS(50));
      if (receiveerror == ESP_OK) {
        //Print the received message
        canTXRXcount[1] += 1;

        if (canTXRXcount[1]%100 == 0){
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
        }
        if (canTXRXcount[1]%100 == 1){
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
        }
        if (canTXRXcount[1]%100 == 2){
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
        }
        if (canTXRXcount[1]%100 == 3){
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
        }
      }
      if (waitforADS == 0){
        Serial.println(receivedCANmessagetoprint);
        receivedCANmessagetoprint = "";
      }
    }
  }
}

void loop() {
  static String loopmessagetoprint;
  String receivedmessage = Serial2.readStringUntil('\n');
  if (receivedmessage != ""){
    loopmessagetoprint += "Received Serial2:";
    loopmessagetoprint += receivedmessage + "\n";
  }
  receivedmessage = Serial.readStringUntil(' ');
  if (receivedmessage == "printADS"){
    loopmessagetoprint += "printADS request: ";
    loopmessagetoprint += receivedmessage + "\n";
    receivedmessage = Serial.readStringUntil(' ');
    int channelnum = receivedmessage.toInt(); 
    if ((channelnum < 4) and (channelnum >= 0)){
      receivedmessage = Serial.readStringUntil('\n');
      if (receivedmessage == "1"){
        printADS[channelnum] = 1;
      }
      else if (receivedmessage == "0"){
        printADS[channelnum] = 0;
      }
    }
  }

  if (waitforADS == 0){
    loopmessagetoprint += "Chip Temp:" + String(temperatureRead()) + "\n";
    Serial.println(loopmessagetoprint);
    loopmessagetoprint = "";
  }
  yield();
}
