#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>

TwoWire I2C_one(0);
ADS1115 ADS(0x48, &I2C_one); // ADS1115 object using TwoWire

const int numSamples = 1024; // Number of samples for FFT
double samplingFrequency = 868.0; // Maximum sampling rate in Hz
double voltageSamples[numSamples];
double vImag[numSamples];

arduinoFFT FFT = arduinoFFT();

void setup() {
  Serial.begin(921600);
  I2C_one.begin(13, 14);
  I2C_one.setClock(400000);

  // Initialize the ADS1115
  if (!ADS.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  ADS.setMode(1);
  ADS.setDataRate(7);
  // Set the gain to the desired range (adjust this based on your voltage range)
  ADS.setGain(0);

  Serial.println("Setup done");

}

void loop() {

  // Store the voltage reading in an array
  static int sampleIndex = 0;

  while (sampleIndex < numSamples) {
    int16_t adc0 = ADS.readADC_Differential_0_1();
    double voltage = (adc0 * 0.1875) / 1000.0; // Calculate voltage in mV
    voltageSamples[sampleIndex] = voltage;
    vImag[sampleIndex]=0;
    Serial.println(voltage);
    sampleIndex++;
  }
  
  // Perform FFT on voltageSamples array
  FFT.Windowing(voltageSamples, numSamples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(voltageSamples, vImag, numSamples, FFT_FORWARD);
  FFT.ComplexToMagnitude(voltageSamples, vImag, numSamples);

  // Find peak frequency and amplitude
  double maxAmplitude = 0;
  int peakFrequencyIndex = 0;
  for (int i = 1; i < numSamples / 2; i++) {
    if (voltageSamples[i] > maxAmplitude) {
      maxAmplitude = voltageSamples[i];
      peakFrequencyIndex = i;
    }
  }

  double peakFrequency = (double)peakFrequencyIndex * samplingFrequency / numSamples;

  /***
  // Print peak frequency and amplitude
  Serial.print("Peak Frequency (Hz): ");
  Serial.println(peakFrequency);
  Serial.print("Amplitude: ");
  Serial.println(maxAmplitude);
  ***/
  
  // Reset sampleIndex
  sampleIndex = 0;
}
