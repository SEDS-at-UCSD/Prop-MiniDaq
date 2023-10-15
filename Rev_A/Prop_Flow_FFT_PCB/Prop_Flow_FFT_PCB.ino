#include <arduinoFFT.h>

#define SAMPLES 512 // Nombre d'échantillons à utiliser pour la FFT
#define SAMPLING_FREQUENCY 3000 // Fréquence d'échantillonnage en Hz

#define FLOWMETER_PIN 11
arduinoFFT FFT = arduinoFFT();

double sampling_period_us;
double vReal[SAMPLES];
double vImag[SAMPLES];

void setup() {
  Serial.begin(115200);

  // Calculer la période d'échantillonnage en microsecondes
  sampling_period_us = 1000000.0 / SAMPLING_FREQUENCY;

}

void loop() {
  // Lire les données analogiques ou numériques ici
  // Vous devez remplir le tableau vReal avec les données d'entrée

  Serial.println("Start Sampling");
  // Exemple : Lire les données analogiques sur la broche A0
  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = analogRead(FLOWMETER_PIN);
    vImag[i] = 0.0;
    delayMicroseconds(1);
  }
  Serial.println("End Sampling");

  // Calculer la FFT
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  // Imprimer les résultats de la FFT
  for (int i = 0; i < SAMPLES / 2; i++) {
    double frequency = (i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
    /***
    Serial.print(frequency);
    Serial.print(" Hz: ");
    Serial.println(vReal[i]);
    ***/
  }
  Serial.print("Max: ");
  double x = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  Serial.println(x, 6);
  Serial.println("next cycle");
  delay(1000); // Attendre 1 seconde entre les mesures
}
