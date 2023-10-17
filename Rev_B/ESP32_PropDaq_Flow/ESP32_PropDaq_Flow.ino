#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>

TwoWire I2C_one(0);
//ADS1015 ADS(0x48, &I2C_one); // ADS1015 object using TwoWire
ADS1115 ADS(0x48, &I2C_one); // ADS1115 object using TwoWire

const int numSamples = 1100; // Number of samples for Counter to store/FFT
double samplingFrequency = 3300; //868; // Maximum sampling rate in Hz, usually 868.0 or 3300
volatile double voltageSamples[numSamples];
volatile double vImag[numSamples];
volatile double min_count[numSamples];

//arduinoFFT FFT = arduinoFFT();
unsigned long startTime;

void setup() {
  Serial.begin(921600);
  //I2C_one.begin(13, 14);
  I2C_one.begin(16, 17); //1015
  //I2C_one.begin(42, 41); //1115
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
  ADS.setGain(16);
  ADS.readADC_Differential_0_1();

  //init to 0
  for (int i = 0; i < numSamples; i++){
    min_count[i] = 0;
  }

}

void loop() {

  // Store the voltage reading in an array
  static int sampleIndex = 0;
  static int count = 0;

  startTime = millis();
  
  int sampling_subcycle = 50;
  for (int i = 0; i < sampling_subcycle; i++){
    int16_t adc0 = ADS.getValue();
    double voltage = (adc0 * 1000* ADS.toVoltage()); // Calculate voltage in mV
    //Serial.print("Raw mV:");
    //Serial.println(voltage);
    if (abs(voltage) < 5){
      voltage = 0;
    } //at least 5mV
    voltageSamples[sampleIndex] = voltage;
    //vImag[sampleIndex]=0;
    
    if (sampleIndex > 1)
    {
      if (min_count[sampleIndex] == 1){ //previous value
        min_count[sampleIndex] = 0;
        count--;
      }
      if ((voltageSamples[sampleIndex] > voltageSamples[sampleIndex-1]) && (voltageSamples[sampleIndex-2] > voltageSamples[sampleIndex-1]))
      { //local minimum
        min_count[sampleIndex] = 1;
        count++;
      }
    }
    //Serial.println(sampleIndex);
    sampleIndex++;
    if (sampleIndex == numSamples){
      sampleIndex = 0;
    }
  }

  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - startTime;
  Serial.print("CountsCPS:");
  Serial.print(sampling_subcycle*1000.0*count/(numSamples*timePassed));
  Serial.print("\t Sampling FPS:");
  Serial.print(sampling_subcycle*1000.0/(timePassed));
  Serial.print("\t PrintFPS:");
  Serial.println(1000.0/(timePassed));
  
}
