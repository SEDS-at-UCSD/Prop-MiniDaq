#include <Wire.h>
#include <ADS1X15.h>
//#include <arduinoFFT.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "driver/twai.h"

// CAN TWAI message to send
twai_message_t txMessage;

// Pins used to connect to CAN bus transceiver:
#define CAN_RX 10
#define CAN_TX 9
int canTXRXcount[2] = {0,0};

StaticJsonDocument<512> sensorData;
DynamicJsonDocument doc0(1024);
DynamicJsonDocument doc1(1024);
DynamicJsonDocument sensor0doc(1024);
DynamicJsonDocument sensor1doc(1024);

TaskHandle_t ADS1015Task;
TaskHandle_t ADS1115Task;

TwoWire I2C_one(0);
TwoWire I2C_two(1);
ADS1015 ADS0(0x48, &I2C_one); // ADS1015 object using TwoWire
ADS1015 ADS1(0x49, &I2C_one); // ADS1015 object using TwoWire
ADS1115 ADS2(0x48, &I2C_two); // ADS1115 object using TwoWire
ADS1115 ADS3(0x49, &I2C_two); // ADS1115 object using TwoWire

const int numSamples = 200; // Number of samples for FFT
double samplingFrequency = 8.0; // Maximum sampling rate in Hz, usually 868.0
volatile double voltageSamples[numSamples];
volatile double vImag[numSamples];

//arduinoFFT FFT = arduinoFFT();
unsigned long startTime0;
unsigned long startTime1;

SemaphoreHandle_t mutex_v;

int baseID = 0x20;

void ADS1015TaskFunction(void* parameter);
void ADS1115TaskFunction(void* parameter);
void sendSerialData(int16_t ADCs[], double data[], String SensorType, int FPS);

void setup() {
  Serial.begin(921600);

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
  // Prepare the message to send
  txMessage.identifier = baseID;           // Example identifier
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

  mutex_v = xSemaphoreCreateMutex(); 
  if (mutex_v == NULL) { 
  Serial.println("Mutex can not be created"); 
  } 

  // sensor0doc["BoardID"] = "DAQ0";
  // sensor1doc["BoardID"] = "DAQ0";

  I2C_one.begin(16, 17);
  I2C_one.setClock(400000);
  I2C_two.begin(42, 41);
  I2C_two.setClock(400000);

  /***
  I2C_one.begin(13, 14);
  I2C_one.setClock(400000);
  I2C_two.begin(5, 4);
  I2C_two.setClock(400000);
  ***/

  startTime0 = millis();
  startTime1 = millis();

  // Initialize the ADS1115
  if (!ADS0.begin()) {
    Serial.println("Failed to initialize ADS0.");
    while (1);
  }
  ADS0.setMode(1);
  ADS0.setDataRate(7);
  // Set the gain to the desired range (adjust this based on your voltage range)
  ADS0.setGain(0);

  if (!ADS1.begin()) {
    Serial.println("Failed to initialize ADS1.");
    while (1);
  }
  ADS1.setMode(1);
  ADS1.setDataRate(7);
  // Set the gain to the desired range (adjust this based on your voltage range)
  ADS1.setGain(0);


  // Initialize the ADS1115
  if (!ADS2.begin()) {
    Serial.println("Failed to initialize ADS2.");
    while (1);
  }
  ADS2.setMode(1);
  ADS2.setDataRate(7);
  // Set the gain to the desired range (adjust this based on your voltage range)
  ADS2.setGain(0);

  if (!ADS3.begin()) {
    Serial.println("Failed to initialize ADS3.");
    while (1);
  }
  ADS3.setMode(1);
  ADS3.setDataRate(7);
  // Set the gain to the desired range (adjust this based on your voltage range)
  ADS3.setGain(0);


  xTaskCreatePinnedToCore(
  ADS1015TaskFunction,
  "ADS1015Task",
  4096,
  NULL,
  1,
  &ADS1015Task,
  0
  );


  xTaskCreatePinnedToCore(
  ADS1115TaskFunction,
  "ADS1115Task",
  4096,
  NULL,
  1,
  &ADS1115Task,
  1
  );

}

void ADS1015TaskFunction(void* parameter) {
  while (1) {
    // Store the voltage reading in an array
    static int sampleIndex = 0;
    int count = 0;

    int16_t adc0 = ADS0.readADC_Differential_0_1();
    int16_t adc0_1 = ADS1.readADC_Differential_0_1();
    int16_t adc1 = ADS0.readADC_Differential_2_3();
    int16_t adc1_1 = ADS1.readADC_Differential_2_3();
  
    int16_t ADC_16bit[4] = {adc0,adc1,adc0_1,adc1_1};

    double voltage0 = (ADS0.toVoltage(adc0)); // Calculate voltage in mV 6.144/2048
    double voltage1 = (ADS0.toVoltage(adc1)); // Calculate voltage in mV 6.144/2048
    double voltage0_1 = (ADS1.toVoltage(adc0_1)); // Calculate voltage in mV
    double voltage1_1 = (ADS1.toVoltage(adc1_1)); // Calculate voltage in mV

    // sensor0doc["PT0"] = voltage0;
    // sensor0doc["PT1"] = voltage1;
    // sensor0doc["PT2"] = voltage0_1;
    // sensor0doc["PT3"] = voltage1_1;
    double data[4] = {voltage0, voltage1, voltage0_1, voltage1_1};

    //Serial.print("In0: "); Serial.print(voltage0); Serial.print("\t In1: "); Serial.print(voltage1);
    //Serial.print("\t In0_1: "); Serial.print(voltage0_1); Serial.print("\t In1_1: "); Serial.print(voltage1_1);
    unsigned long currentTime = millis();
    unsigned long timePassed = currentTime - startTime0;
    startTime0 = millis();
    sampleIndex = 0;
    //String sensorjson0;
    xSemaphoreTake(mutex_v, portMAX_DELAY); 
    // serializeJson(sensor0doc, Serial);
    sendSerialData(ADC_16bit, data, "ADS1015", 1000*1/(timePassed));
    //Serial.print("\t FPS: "); Serial.println(1000*1/(timePassed));
    xSemaphoreGive(mutex_v); 
  }
}

void ADS1115TaskFunction(void* parameter) {
  while (1) {
    // Store the voltage reading in an array
    static int sampleIndex = 0;
    int count = 0;
    int16_t adc0 = ADS2.readADC_Differential_0_1();
    int16_t adc0_1 = ADS3.readADC_Differential_0_1();

    int16_t adc1 = ADS2.readADC_Differential_2_3();
    int16_t adc1_1 = ADS3.readADC_Differential_2_3();

    int16_t ADC_16bit[4] = {adc0,adc1,adc0_1,adc1_1};
    
    //Serial.print("In0: "); Serial.print(voltage0); Serial.print("\t In1: "); Serial.print(voltage1);

    double voltage0 = (ADS2.toVoltage(adc0)); // Calculate voltage in mV 6.144/32767
    double voltage1 = (ADS3.toVoltage(adc1)); // Calculate voltage in mV 6.144/32767
    double voltage0_1 = (ADS2.toVoltage(adc0_1)); // Calculate voltage in mV
    double voltage1_1 = (ADS3.toVoltage(adc1_1)); // Calculate voltage in mV

    double data[4] = {voltage0, voltage1, voltage0_1, voltage1_1};
    //Serial.print("\t In0_1: "); Serial.print(voltage0_1); Serial.print("\t In1_1: "); Serial.print(voltage1_1);
    unsigned long currentTime = millis();
    unsigned long timePassed = currentTime - startTime1;
    startTime1 = millis();
    sampleIndex = 0;
    xSemaphoreTake(mutex_v, portMAX_DELAY); 
    // serializeJson(sensor1doc, Serial);
    sendSerialData(ADC_16bit, data, "ADS1115", 1000*1/(timePassed));
    // Serial.print("\t FPS: "); Serial.println(1000*1/(timePassed));
    xSemaphoreGive(mutex_v); 
  }
}

void loop() {
  //doc["BoardID"] = "PT Board";
}

void sendSerialData(int16_t ADC_16bit[], double data[], String SensorType, int FPS) {
  //JSON doc
  int data_arr_size = 4;

  sensorData["BoardID"] = "Board 1";
  sensorData["SensorType"] = SensorType;

  if (SensorType == "ADS1015"){
    txMessage.identifier = baseID + 0x01;      
  }
  if (SensorType == "ADS1115"){
    txMessage.identifier = baseID + 0x02;      
  }

  for (int i = 0; i < data_arr_size; i++) {
    sensorData["Sensors"][i] = data[i];
    txMessage.data[2*i] = (ADC_16bit[i] >> 8) & 0xFF;  // High byte
    txMessage.data[2*i+1] = ADC_16bit[i] & 0xFF;       // Low byte
  }
  // doc["Sensors"][1] = data[1];
  sensorData["FPS"] = FPS;

  serializeJson(sensorData, Serial);
  Serial.println();
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
}