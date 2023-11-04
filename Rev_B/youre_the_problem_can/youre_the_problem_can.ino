#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "driver/twai.h"

// --------------------------- CAN --------------------------- //
// Task handles
TaskHandle_t transmitTaskHandle;
TaskHandle_t receiveTaskHandle;

// Function prototypes
void transmitTask(void *pvParameters);
void receiveTask(void *pvParameters);

// CAN TWAI message to send
twai_message_t txMessage_1015;
twai_message_t txMessage_1115;

// Pins used to connect to CAN bus transceiver:
#define CAN_RX 10
#define CAN_TX 9


// --------------------------- JSON --------------------------- //

StaticJsonDocument<512> sensorData;
DynamicJsonDocument doc0(1024);
DynamicJsonDocument doc1(1024);
DynamicJsonDocument sensor0doc(1024);
DynamicJsonDocument sensor1doc(1024);

// --------------------------- RTOS --------------------------- //

TaskHandle_t ADS1015Task;
TaskHandle_t ADS1115Task;

// --------------------------- I2C --------------------------- //
TwoWire I2C_one(0);
TwoWire I2C_two(1);
ADS1015 ADS0(0x48, &I2C_one); // ADS1015 object using TwoWire
ADS1015 ADS1(0x49, &I2C_one); // ADS1015 object using TwoWire
ADS1115 ADS2(0x48, &I2C_two); // ADS1115 object using TwoWire
ADS1115 ADS3(0x49, &I2C_two); // ADS1115 object using TwoWire

ADS1X15 ADS[] = {ADS0, ADS1, ADS2, ADS3};

const int numSamples = 200; // Number of samples for FFT
double samplingFrequency = 8.0; // Maximum sampling rate in Hz, usually 868.0
volatile double voltageSamples[numSamples];
volatile double vImag[numSamples];

// --------------------------- DUMB PERSON --------------------------- //
unsigned long startTime0;
unsigned long startTime1;

SemaphoreHandle_t mutex_v;

void ADS1015TaskFunction(void* parameter);
void ADS1115TaskFunction(void* parameter);

void setup() {
  Serial.begin(921600);

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

  for (int i = 0; i < 4; i++) {
    if (!ADS[i].begin()) {
      Serial.println("Failed to initialize ADS " + i);
      while (1);
    }
    ADS[i].setMode(1);
    ADS[i].setDataRate(7);
    // Set the gain to the desired range (adjust this based on your voltage range)
    ADS[i].setGain(0);
  }

// --------------- CAN Init --------------- //
  pinMode(CAN_TX,OUTPUT);
  pinMode(CAN_RX,INPUT);

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install and start the TWAI driver
  esp_err_t canStatus = twai_driver_install(&g_config, &t_config, &f_config);
  if (canStatus == ESP_OK) {
    Serial.println("\nCAN Driver installed");
  } else {
    Serial.println("\nCAN Driver installation failed");
  }
  twai_start();

// --------------------------- RTOS --------------------------- //

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
    
    //Serial.print("In0: "); Serial.print(voltage0); Serial.print("\t In1: "); Serial.print(voltage1);

    double voltage0 = (ADS0.toVoltage(adc0)); // Calculate voltage in mV 6.144/2048
    double voltage1 = (ADS0.toVoltage(adc1)); // Calculate voltage in mV 6.144/2048
    double voltage0_1 = (ADS1.toVoltage(adc0_1)); // Calculate voltage in mV
    double voltage1_1 = (ADS1.toVoltage(adc1_1)); // Calculate voltage in mV

    int16_t adc_data[4] = {adc0, adc0_1, adc1, adc1_1};
    double voltage_data[4] = {voltage0, voltage1, voltage0_1, voltage1_1};

    uint8_t can_data[8];
    for (int i = 0; i < 4; i++) {
      can_data[i * 2 + 0] = highByte(adc_data[i]);
      can_data[i * 2 + 1] = lowByte(adc_data[i]);
    }

    //Serial.print("\t In0_1: "); Serial.print(voltage0_1); Serial.print("\t In1_1: "); Serial.print(voltage1_1);
    unsigned long currentTime = millis();
    unsigned long timePassed = currentTime - startTime0;
    startTime0 = millis();
    sampleIndex = 0;
    //String sensorjson0;
    xSemaphoreTake(mutex_v, portMAX_DELAY); 
    // serializeJson(sensor0doc, Serial);
    sendSerialData(voltage_data, "ADS1015", 1000*1/(timePassed));

    // uint8_t can_data[8] = {0x51, 0x51, 0x13, 0x21, 0x12, 0xFF, 0xFF, 0xFF};

    setData(&txMessage_1015, 0x13, TWAI_MSG_FLAG_EXTD, 8, can_data);
    twai_transmit(&txMessage_1015, pdMS_TO_TICKS(1)); 
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
    
    //Serial.print("In0: "); Serial.print(voltage0); Serial.print("\t In1: "); Serial.print(voltage1);

    double voltage0 = (ADS2.toVoltage(adc0)); // Calculate voltage in mV 6.144/32767
    double voltage1 = (ADS3.toVoltage(adc1)); // Calculate voltage in mV 6.144/32767
    double voltage0_1 = (ADS2.toVoltage(adc0_1)); // Calculate voltage in mV
    double voltage1_1 = (ADS3.toVoltage(adc1_1)); // Calculate voltage in mV

    int16_t adc_data[4] = {adc0, adc0_1, adc1, adc1_1};
    double voltage_data[4] = {voltage0, voltage1, voltage0_1, voltage1_1};

    uint8_t can_data[8];
    for (int i = 0; i < 4; i++) {
      can_data[i * 2 + 0] = highByte(adc_data[i]);
      can_data[i * 2 + 1] = lowByte(adc_data[i]);
    }

    //Serial.print("\t In0_1: "); Serial.print(voltage0_1); Serial.print("\t In1_1: "); Serial.print(voltage1_1);
    unsigned long currentTime = millis();
    unsigned long timePassed = currentTime - startTime1;
    startTime1 = millis();
    sampleIndex = 0;
    xSemaphoreTake(mutex_v, portMAX_DELAY); 
    // serializeJson(sensor1doc, Serial);
    sendSerialData(voltage_data, "ADS1115", 1000*1/(timePassed));
    // Serial.print("\t FPS: "); Serial.println(1000*1/(timePassed));


    setData(&txMessage_1115, 0x12, TWAI_MSG_FLAG_EXTD, 8, can_data);
    twai_transmit(&txMessage_1115, pdMS_TO_TICKS(1));

    xSemaphoreGive(mutex_v); 
  }
}

void loop() {
  //doc["BoardID"] = "PT Board";
}

void sendSerialData(double data[], String SensorType, int FPS) {
  //JSON doc
  int data_arr_size = 4;

  sensorData["BoardID"] = "Board 1";
  sensorData["SensorType"] = SensorType;

  for (int i = 0; i < data_arr_size; i++) {
    sensorData["Sensors"][i] = data[i];
  }
  // doc["Sensors"][1] = data[1];
  sensorData["FPS"] = FPS;

  serializeJson(sensorData, Serial);
  Serial.println();
}


void setData(twai_message_t *txMessage, uint32_t id, uint32_t flags, uint8_t data_length_code, uint8_t *data) {
  txMessage->identifier = id;           // Example identifier
  txMessage->flags = flags;  // Example flags (extended frame)
  txMessage->data_length_code = data_length_code;        // Example data length (8 bytes)

  // txMessage.data = data;
  for (int i = 0 ; i < 8; i++) {
    txMessage->data[i] = data[i];
  }
}
