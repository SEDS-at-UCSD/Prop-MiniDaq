#include <Wire.h>
#include <ADS1X15.h>
#include <arduinoFFT.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define task handles and queue handle
TaskHandle_t dataCollectionTask, fftProcessingTask;
QueueHandle_t dataQueue;

TwoWire I2C_one(0);
ADS1115 ADS(0x48, &I2C_one);

const int numSamples = 1024;
double samplingFrequency = 868.0;
double voltageSamples[numSamples];
double vImag[numSamples];

arduinoFFT FFT = arduinoFFT();

void dataCollectionTaskFunction(void* parameter) {
  while (1) {
    int sampleIndex = 0;
    while (sampleIndex < numSamples) {
      int16_t adc0 = ADS.readADC_Differential_0_1();
      double voltage = (adc0 * 0.1875) / 1000.0;
      voltageSamples[sampleIndex] = voltage;
      vImag[sampleIndex] = 0;
      sampleIndex++;
    }

    xQueueSend(dataQueue, voltageSamples, portMAX_DELAY);
  }
}

void fftProcessingTaskFunction(void* parameter) {
  while (1) {
    double receivedData[numSamples];

    if (xQueueReceive(dataQueue, receivedData, portMAX_DELAY)) {
      FFT.Windowing(receivedData, numSamples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
      FFT.Compute(receivedData, vImag, numSamples, FFT_FORWARD);
      FFT.ComplexToMagnitude(receivedData, vImag, numSamples);

      double maxAmplitude = 0;
      int peakFrequencyIndex = 0;
      for (int i = 1; i < numSamples / 2; i++) {
        if (receivedData[i] > maxAmplitude) {
          maxAmplitude = receivedData[i];
          peakFrequencyIndex = i;
        }
      }

      double peakFrequency = (double)peakFrequencyIndex * samplingFrequency / numSamples;

      Serial.print("Peak Frequency (Hz): ");
      Serial.println(peakFrequency);
      Serial.print("Amplitude: ");
      Serial.println(maxAmplitude);
    }
  }
}

void setup() {
  Serial.begin(921600);
  I2C_one.begin(13, 14);
  I2C_one.setClock(400000);

  if (!ADS.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  ADS.setGain(0);

  dataQueue = xQueueCreate(1, numSamples * sizeof(double));

  xTaskCreatePinnedToCore(
    dataCollectionTaskFunction,
    "DataCollectionTask",
    4096,
    NULL,
    1,
    &dataCollectionTask,
    0
  );

  xTaskCreatePinnedToCore(
    fftProcessingTaskFunction,
    "FFTProcessingTask",
    4096,
    NULL,
    1,
    &fftProcessingTask,
    1
  );

  vTaskStartScheduler();
}

void loop() {
  // This code should not contain any tasks; it will not run in this example.
}
