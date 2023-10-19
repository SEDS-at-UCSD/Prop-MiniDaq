#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

//DynamicJsonDocument doc(1024);
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

void ADS1015TaskFunction(void* parameter);
void ADS1115TaskFunction(void* parameter);

void setup() {
  Serial.begin(921600);

  mutex_v = xSemaphoreCreateMutex(); 
  if (mutex_v == NULL) { 
  Serial.println("Mutex can not be created"); 
  } 

  sensor0doc["BoardID"] = "DAQ0";
  sensor1doc["BoardID"] = "DAQ0";

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
    
    //Serial.print("In0: "); Serial.print(voltage0); Serial.print("\t In1: "); Serial.print(voltage1);

    double voltage0 = (ADS0.toVoltage(adc0)); // Calculate voltage in mV 6.144/2048
    double voltage1 = (ADS0.toVoltage(adc1)); // Calculate voltage in mV 6.144/2048
    double voltage0_1 = (ADS1.toVoltage(adc0_1)); // Calculate voltage in mV
    double voltage1_1 = (ADS1.toVoltage(adc1_1)); // Calculate voltage in mV
    sensor0doc["PT0"] = voltage0;
    sensor0doc["PT1"] = voltage1;
    sensor0doc["PT2"] = voltage0_1;
    sensor0doc["PT3"] = voltage1_1;

    //Serial.print("\t In0_1: "); Serial.print(voltage0_1); Serial.print("\t In1_1: "); Serial.print(voltage1_1);
    unsigned long currentTime = millis();
    unsigned long timePassed = currentTime - startTime0;
    startTime0 = millis();
    sampleIndex = 0;
    //String sensorjson0;
    xSemaphoreTake(mutex_v, portMAX_DELAY); 
    serializeJson(sensor0doc, Serial);
    Serial.print("\t FPS: "); Serial.println(1000*1/(timePassed));
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

    double voltage0 = (adc0 * 0.1875); // Calculate voltage in mV 6.144/32767
    double voltage1 = (adc1 * 0.1875); // Calculate voltage in mV
    double voltage0_1 = (adc0_1 * (0.1875)); // Calculate voltage in mV
    double voltage1_1 = (adc1_1 * (0.1875)); // Calculate voltage in mV

    sensor1doc["TC0"] = voltage0;
    sensor1doc["TC1"] = voltage1;
    sensor1doc["TC2"] = voltage0_1;
    sensor1doc["TC3"] = voltage1_1;
    //Serial.print("\t In0_1: "); Serial.print(voltage0_1); Serial.print("\t In1_1: "); Serial.print(voltage1_1);
    unsigned long currentTime = millis();
    unsigned long timePassed = currentTime - startTime1;
    startTime1 = millis();
    sampleIndex = 0;
    xSemaphoreTake(mutex_v, portMAX_DELAY); 
    serializeJson(sensor1doc, Serial);
    Serial.print("\t FPS: "); Serial.println(1000*1/(timePassed));
    xSemaphoreGive(mutex_v); 
  }
}

void loop() {
  //doc["BoardID"] = "PT Board";
}
