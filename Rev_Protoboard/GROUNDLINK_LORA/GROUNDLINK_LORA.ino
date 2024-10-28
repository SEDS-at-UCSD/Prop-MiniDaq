#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// LoRa Parameters for North America
#define FREQUENCY           905.2
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    9
#define TRANSMIT_POWER      1

// Global variables
String rxData;
volatile bool rxFlag = false;
SemaphoreHandle_t loraMutex;

// Function prototypes
void loraReceiveTask(void *pvParameters);
void serialControlTask(void *pvParameters);
void rxCallback();

// Task handles
TaskHandle_t loraReceiveTaskHandle;
TaskHandle_t serialControlTaskHandle;

void setup() {
  Serial.begin(921600);
  heltec_setup();  // Initialize Heltec
  both.println("GroundLink init");

  // LoRa settings
  RADIOLIB_OR_HALT(radio.begin());
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TRANSMIT_POWER);
  radio.setDio1Action(rxCallback);
  radio.startReceive();

  // Initialize the mutex
  loraMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(loraReceiveTask, "LoRa Receive Task", 4096, NULL, 1, &loraReceiveTaskHandle, 0);
  xTaskCreatePinnedToCore(serialControlTask, "Serial Control Task", 4096, NULL, 1, &serialControlTaskHandle, 1);
}

void loraReceiveTask(void *pvParameters) {
  StaticJsonDocument<512> doc;
  while (1) {
    if (rxFlag) {
      rxFlag = false;

      if (xSemaphoreTake(loraMutex, portMAX_DELAY)) {  // Take the mutex
        int status = radio.readData(rxData);  // Check for read success
        xSemaphoreGive(loraMutex);

        if (status == RADIOLIB_ERR_NONE) {
          rxData.trim();
          if (rxData.startsWith("SKYLINK:") && rxData.endsWith(":END")) {
            String innerData = rxData.substring(8, rxData.length() - 4);  // Extract content
            Serial.printf("Verified RX: %s\n", innerData.c_str());
          } else {
            Serial.printf("Unverified RX format: %s\n", rxData.c_str());
          }

          if (deserializeJson(doc, rxData) == DeserializationError::Ok) {
            serializeJson(doc, Serial);
            Serial.println();
          } else {
            Serial.println("JSON Parse Error");
          }
        } else {
          Serial.printf("LoRa read error: %d\n", status);
        }
      }

      rxData = "";  // Clear rxData for the next message

      vTaskDelay(pdMS_TO_TICKS(10));
      if (xSemaphoreTake(loraMutex, portMAX_DELAY)) {
        radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
        xSemaphoreGive(loraMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Task to read Serial input and send over LoRa
void serialControlTask(void *pvParameters) {
  while (1) {
    if (Serial.available()) {
      String command = Serial.readStringUntil('\n');
      String message = "GROUNDLINK: " + command + " :END\n";

      if (xSemaphoreTake(loraMutex, portMAX_DELAY)) {  // Take the mutex
        heltec_led(50);
        radio.transmit(message.c_str());
        heltec_led(0);
        xSemaphoreGive(loraMutex);  // Release the mutex
        vTaskDelay(pdMS_TO_TICKS(10));  // Short delay for stability
      }

      Serial.printf("SENT: %s\n", message.c_str());
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Callback for LoRa reception
void rxCallback() {
  rxFlag = true;
}

void loop() {
  heltec_loop();
}
