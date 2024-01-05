// Basic example using one-shot measurement.
// The call to readThermocoupleTemperature() is blocking for O(100ms)

#include <Adafruit_MAX31856.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"

// CAN TWAI message to send
twai_message_t txMessage;
// Pins used to connect to CAN bus transceiver:
#define CAN_RX 10
#define CAN_TX 9
int canTXRXcount[2] = {0,0};
int baseID_int = 1;
int baseID  = (0x10)*baseID_int;

StaticJsonDocument<512> sensorData;

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31856 maxthermo0 = Adafruit_MAX31856(35, 3, 48, 45);
Adafruit_MAX31856 maxthermo1 = Adafruit_MAX31856(36, 3, 48, 45);
Adafruit_MAX31856 maxthermo2 = Adafruit_MAX31856(37, 3, 48, 45);
Adafruit_MAX31856 maxthermo3 = Adafruit_MAX31856(38, 3, 48, 45);
Adafruit_MAX31856 maxthermo4 = Adafruit_MAX31856(39, 3, 48, 45);

Adafruit_MAX31856 maxthermo[4] = {maxthermo0,maxthermo1,maxthermo2,maxthermo3};
max31856_thermocoupletype_t MAXCases[4] = {MAX31856_TCTYPE_K,MAX31856_TCTYPE_K,MAX31856_TCTYPE_K,MAX31856_TCTYPE_T};
bool MAXActive[4] = {1,1,1,1};

unsigned long startTime;

int CSPINS[] = {35,36,37,38,39};
// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10);

void setup() {
  Serial.begin(921600);
  while (!Serial) delay(10);

  // CAN
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

  txMessage.identifier = baseID + 0x03;           // Solenoid Board identifier Board 0x5X
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

  //Serial.println("MAX31856 thermocouple test");

  for (int i = 0; i < 5; i++){
    pinMode(CSPINS[i],OUTPUT);
    digitalWrite(CSPINS[i],HIGH);
  }
  
  for (int i = 0; i < 4 ; i++){

    maxthermo[i].begin();

    maxthermo[i].setThermocoupleType(MAXCases[i]);

    Serial.print("Thermocouple type: ");
    switch (maxthermo[i].getThermocoupleType() ) {
      case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
      case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
      case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
      case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
      case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
      case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
      case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
      case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
      case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
      case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
      default: Serial.println("Unknown"); break;
    }
  }

}

void loop() {
  sensorData["BoardID"] = "Board " + String(baseID_int);
  sensorData["SensorType"] = "Thermocouple";
  for (int i = 0; i <  4 ; i++){
    
      if (MAXActive[i]){
      //Serial.print("Sensor: ");
      //Serial.print(i);
      //Serial.print("\t Cold Junction Temp: ");
      //Serial.println(maxthermo[i].readCJTemperature());
      // Check and print any faults
      uint8_t fault = maxthermo[i].readFault();
      if (fault) {
        delay(10);
        sensorData["Sensors"][i] = -274;
        txMessage.data[2*i] = 0xFF;
        txMessage.data[2*i+1] = 0xFF;
        /*
        if (fault & MAX31856_FAULT_CJRANGE) Serial.println("Cold Junction Range Fault");
        if (fault & MAX31856_FAULT_TCRANGE) Serial.println("Thermocouple Range Fault");
        if (fault & MAX31856_FAULT_CJHIGH)  Serial.println("Cold Junction High Fault");
        if (fault & MAX31856_FAULT_CJLOW)   Serial.println("Cold Junction Low Fault");
        if (fault & MAX31856_FAULT_TCHIGH)  Serial.println("Thermocouple High Fault");
        if (fault & MAX31856_FAULT_TCLOW)   Serial.println("Thermocouple Low Fault");
        if (fault & MAX31856_FAULT_OVUV)    Serial.println("Over/Under Voltage Fault");
        if (fault & MAX31856_FAULT_OPEN)    Serial.println("Thermocouple Open Fault");
        */
      } else{
        //Serial.print("Thermocouple Temp: ");
        //Serial.println(maxthermo[i].readThermocoupleTemperature());
        sensorData["Sensors"][i] = maxthermo[i].readThermocoupleTemperature();txMessage.data[i] ;
        uint16_t value = floatToIEEE754(sensorData["Sensors"][i]);
        txMessage.data[2*i] = value/(256);              // Reserved for message type
        txMessage.data[2*i+1] = (value - (txMessage.data[2*i]*(256)));

      }
    }
  }
  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - startTime;
  startTime = millis();
  sensorData["FPS"] = 1000.0/(timePassed);
  serializeJson(sensorData, Serial);
  Serial.println();
  twai_transmit(&txMessage, pdMS_TO_TICKS(1));
  //Serial.print("\t FPS: "); //Serial.println(1000.0/(timePassed),3);
}

uint16_t floatToIEEE754(float value) {
  uint32_t intValue = *(uint32_t*)&value; // Interpret the float as an unsigned 32-bit integer

  // Extract the sign, exponent, and mantissa from the 32-bit integer
  uint16_t sign = (intValue >> 31) & 0x1;
  uint16_t exponent = ((intValue >> 23) & 0xFF) - 127;
  uint16_t mantissa = (intValue & 0x7FFFFF) >> 16;

  // Combine the sign, exponent, and mantissa into a 16-bit IEEE 754 representation
  uint16_t ieee754Value = (sign << 15) | ((exponent + 15) << 10) | mantissa;

  return ieee754Value;
}