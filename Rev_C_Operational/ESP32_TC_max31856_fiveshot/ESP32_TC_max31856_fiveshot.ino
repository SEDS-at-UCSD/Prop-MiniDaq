// Basic example using one-shot measurement.
// The call to readThermocoupleTemperature() is blocking for O(100ms)

#include <Adafruit_MAX31856.h>
#include <ArduinoJson.h>

StaticJsonDocument<512> sensorData;

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31856 maxthermo0 = Adafruit_MAX31856(35, 3, 48, 45);
Adafruit_MAX31856 maxthermo1 = Adafruit_MAX31856(36, 3, 48, 45);
Adafruit_MAX31856 maxthermo2 = Adafruit_MAX31856(37, 3, 48, 45);
Adafruit_MAX31856 maxthermo3 = Adafruit_MAX31856(38, 3, 48, 45);
Adafruit_MAX31856 maxthermo4 = Adafruit_MAX31856(39, 3, 48, 45);

Adafruit_MAX31856 maxthermo[] = {maxthermo0,maxthermo1,maxthermo2,maxthermo3,maxthermo4};
max31856_thermocoupletype_t MAXCases[] = {MAX31856_TCTYPE_K,MAX31856_TCTYPE_K,MAX31856_TCTYPE_K,MAX31856_TCTYPE_T,MAX31856_TCTYPE_T};
bool MAXActive[] = {1,1,1,1,1};

int baseID_int = 4;
unsigned long startTime;

int CSPINS[] = {35,36,37,38,39};
// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10);

void setup() {
  Serial.begin(921600);
  while (!Serial) delay(10);
  //Serial.println("MAX31856 thermocouple test");

  for (int i = 0; i < 5; i++){
    pinMode(CSPINS[i],OUTPUT);
    digitalWrite(CSPINS[i],HIGH);
  }
  
  for (int i = 0; i < 5; i++){

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
  for (int i = 0; i < 5; i++){
    
      if (MAXActive[i]){
      //Serial.print("Sensor: ");
      //Serial.print(i);
      //Serial.print("\t Cold Junction Temp: ");
      //Serial.println(maxthermo[i].readCJTemperature());
      // Check and print any faults
      uint8_t fault = maxthermo[i].readFault();
      if (fault) {
        sensorData["Sensors"][i] = -273;
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
        sensorData["Sensors"][i] = maxthermo[i].readThermocoupleTemperature();
      }
    }
  }
  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - startTime;
  startTime = millis();
  sensorData["FPS"] = 1000.0/(timePassed);
  serializeJson(sensorData, Serial);
  Serial.println();
  delay(1);
  //Serial.print("\t FPS: "); //Serial.println(1000.0/(timePassed),3);
}