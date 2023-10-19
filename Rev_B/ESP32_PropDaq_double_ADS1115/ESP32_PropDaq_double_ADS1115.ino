#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>

TwoWire I2C_one(0);
TwoWire I2C_two(1);
ADS1015 ADS0(0x48, &I2C_one); // ADS1115 object using TwoWire
ADS1015 ADS1(0x48, &I2C_one); // ADS1115 object using TwoWire

const int numSamples = 200; // Number of samples for FFT
double samplingFrequency = 8.0; // Maximum sampling rate in Hz, usually 868.0
volatile double voltageSamples[numSamples];
volatile double vImag[numSamples];

//arduinoFFT FFT = arduinoFFT();
unsigned long startTime;

void setup() {
  Serial.begin(921600);
  I2C_one.begin(16, 17);
  I2C_one.setClock(400000);
  I2C_two.begin(42, 41);
  I2C_two.setClock(400000);
  startTime = millis();

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
  ADS1.setGain(8);

}

void loop() {

  // Store the voltage reading in an array
  static int sampleIndex = 0;
  int count = 0;

  startTime = millis();
  int16_t adc0 = ADS0.readADC_Differential_0_1();
  int16_t adc1 = ADS0.readADC_Differential_2_3();
  double voltage0 = ADS0.toVoltage(adc0); // Calculate voltage in mV 6.144/32767
  double voltage1 = ADS0.toVoltage(adc1); // Calculate voltage in mV
  Serial.print("In0: ");
  Serial.print(voltage0);
  Serial.print("\t In1: ");
  Serial.print(voltage1);

  int16_t adc0_1 = ADS1.readADC_Differential_0_1();
  int16_t adc1_1 = ADS1.readADC_Differential_2_3();
  double voltage0_1 = ADS1.toVoltage(adc0_1); //(adc0_1 * (512/32767.0)); // Calculate voltage in mV
  double voltage1_1 = ADS1.toVoltage(adc1_1); //(adc1_1 * (512/32767.0)); // Calculate voltage in mV
  Serial.print("\t In0_1: ");
  Serial.print(voltage0_1);
  Serial.print("\t In1_1: ");
  Serial.print(voltage1_1);

  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - startTime;
  Serial.print("\t FPS: ");
  Serial.println(1000*1/(timePassed));
  sampleIndex = 0;
}
