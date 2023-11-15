#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>
#include <ArduinoJson.h>

StaticJsonDocument<512> sensorData;

TwoWire I2C_one(0);
ADS1015 ADS(0x48, &I2C_one); // ADS1115 object using TwoWire

//arduinoFFT FFT = arduinoFFT();
unsigned long startTime;
unsigned long currentTime;
unsigned long timePassed;

void setup() {
  Serial.begin(921600);
  I2C_one.begin(13, 14);
  //I2C_one.begin(16, 17);
  //I2C_one.begin(5, 4);
  I2C_one.setClock(400000);
  startTime = millis();

  // Initialize the ADS1115
  if (!ADS.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  ADS.setMode(0);
  ADS.setDataRate(7);
  // Set the gain to the desired range (adjust this based on your voltage range)
  ADS.setGain(0);
  ADS.readADC_Differential_0_1();

}

void loop() {

  // Store the voltage reading in an array
  static int sampleIndex = 0;
  static int count = 0;

  startTime = millis();
  
  int sampling_subcycle = 1000;
  int samplefps = sampling_subcycle*1000.0/(timePassed);

  sensorData["BoardID"] = "Flow";
  sensorData["SensorType"] = "mV";

  for (int i = 0; i < sampling_subcycle; i++){
    int16_t adc0 = ADS.getValue();
    double voltage = (ADS.toVoltage(adc0)); // Calculate voltage in mV
    
    String outputstr = "{\"BoardID\":\"Flow\",\"mV\":";
    outputstr += String(voltage);
    outputstr += ", \"FPS\":";
    outputstr += String(samplefps);
    outputstr += "}";

    Serial.println(outputstr);
    
    /*
    sensorData["Sensors"] = voltage;
    sensorData["FPS"] = samplefps;
    serializeJson(sensorData, Serial);
    Serial.println();
    */
    /*
    Serial.print("V:");
    Serial.print(voltage);
    Serial.print("\t Sampling FPS:");
    Serial.println(samplefps);
    */

    
  }

  currentTime = millis();
  timePassed = currentTime - startTime;
  
}
