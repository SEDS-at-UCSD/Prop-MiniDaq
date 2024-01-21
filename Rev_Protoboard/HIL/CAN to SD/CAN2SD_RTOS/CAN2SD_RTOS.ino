#include <SPI.h>
#include <SD.h>
#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Define CAN pins and SD card pins
#define CAN_TX 1
#define CAN_RX 2
int CSpins[4] = {21,45,47,48};
SPIClass SD_SPI(HSPI);
#define SD_CS_PIN 45
#define SD_MOSI_PIN 8
#define SD_MISO_PIN 9
#define SD_SCK_PIN 7

// Shared variables
QueueHandle_t canQueue;
SemaphoreHandle_t sdMutex;

// Function declarations
void canTask(void *parameter);
void sdCardTask(void *parameter);
void setupCan();
void setupSdCard();
float ieee754ToFloat(uint16_t ieee754Value);
void saveMessageToFile(const twai_message_t& message);

void setup() {
  Serial.begin(921600);
  pinMode(RGB_BUILTIN, OUTPUT);
   neopixelWrite(RGB_BUILTIN,0,0,0); // Off
  setupCan();
  delay(100);
  setupSdCard();
  delay(100);

  // Create a queue for CAN messages
  canQueue = xQueueCreate(10, sizeof(twai_message_t));

  // Create a mutex for SD card access
  sdMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(canTask, "CAN_Task", 10000, NULL, 1, NULL, 0); // Task for CAN, Core 0
  xTaskCreatePinnedToCore(sdCardTask, "SD_Task", 10000, NULL, 1, NULL, 1); // Task for SD Card, Core 1
}

void loop() {
  // Main loop does nothing, tasks handle everything
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void canTask(void *parameter) {
  twai_message_t message;
  while (1) {
    if (twai_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
      xQueueSend(canQueue, &message, portMAX_DELAY);
    }
  }
}

void sdCardTask(void *parameter) {
  twai_message_t message;
  while (1) {
    if (xQueueReceive(canQueue, &message, portMAX_DELAY) == pdTRUE) {
      // Lock the mutex before accessing the SD card
      if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        saveMessageToFile(message);

        // Release the mutex after done
        xSemaphoreGive(sdMutex);
      }
    }
  }
}

void setupCan() {
  // Initialize CAN interface
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t canStatus = twai_driver_install(&g_config, &t_config, &f_config);
  if (canStatus == ESP_OK) {
    Serial.println("CAN Driver installed");
  } else {
    Serial.println("CAN Driver installation failed");
  }

  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver");
    return;
  }
  Serial.println("TWAI driver started");
}

void setupSdCard() {
  // Initialize SD card
  for (int i=0; i<4; i++){
    digitalWrite(CSpins[i], HIGH);
    delay(10);
  }
  digitalWrite(SD_CS_PIN, LOW);
  SD_SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SD_SPI)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");
  File dataFile = SD.open("/can_data.txt", FILE_APPEND, true);
  if (dataFile) {
    unsigned long timestamp = millis();
    dataFile.print("Time: ");
    dataFile.print(timestamp);
    dataFile.print(", --- SD Card Re-Initialization Triggered --- ");
    dataFile.println();
    dataFile.close();
  } else {
    Serial.println("Error opening file");
    return;
  }
  neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,0); // Off
  delay(100);

}


float ieee754ToFloat(uint16_t ieee754Value) {
    // Extract the sign, exponent, and mantissa from the 16-bit integer
    uint32_t sign = (ieee754Value >> 15) & 0x1;
    int32_t exponent = ((ieee754Value >> 10) & 0x1F) - 15;
    uint32_t mantissa = ieee754Value & 0x3FF;

    // Adjust the exponent and mantissa for the 32-bit float format
    exponent += 127;
    mantissa <<= 13; // Align the mantissa to the 23-bit format

    // Handle special cases for exponent
    if (exponent <= 0) {
        // Denormalized number or zero
        exponent = 0;
    } else if (exponent >= 255) {
        // Overflow (infinity)
        exponent = 255;
        mantissa = 0;
    }

    // Combine the sign, exponent, and mantissa into a 32-bit representation
    uint32_t intValue = (sign << 31) | ((exponent & 0xFF) << 23) | (mantissa & 0x7FFFFF);

    // Reinterpret the bits as a float
    float value;
    memcpy(&value, &intValue, sizeof(float)); // Use memcpy to avoid breaking strict aliasing rules

    return value;
}

void saveMessageToFile(const twai_message_t& message) {
  // Open file for writing
  File dataFile = SD.open("/can_data.txt", FILE_APPEND, true);
  // If the file is available, write to it
  if (dataFile) {
    unsigned long timestamp = millis(); // Estimated timestamp
    dataFile.print("Time: ");
    dataFile.print(timestamp);
    dataFile.print(", ID: ");
    dataFile.print(message.identifier, HEX);
    dataFile.print(", Data: ");
    for (int i = 0; i < message.data_length_code; i++) {
      dataFile.print(message.data[i], HEX);
      dataFile.print(" ");
    }
    dataFile.print(", Converted: ");
    if (message.identifier < 0x40){ //boards 1-3
      if ((message.identifier % 0x10) == 1){ //sensor ID 1: ADS1015
        Serial.print("ADS1015: ");
        for (int i = 0; i < message.data_length_code; i+=2) {
          int16_t msg_value = (uint8_t)(message.data[i+1]) | ((uint8_t)message.data[i] << 8);
          //int msg_value = (uint8_t)message.data[2*i]*0xFF + (uint8_t)message.data[2*i+1];
          float value = msg_value*(6.144/(5*2048));
          Serial.print(value);
          Serial.print(" ");
          dataFile.print(value);
          dataFile.print(" ");
        }
      } else if ((message.identifier % 0x10) == 2){ //sensor ID 2: ADS1115
        Serial.print("ADS1115: ");
        for (int i = 0; i < message.data_length_code; i+=2) {
          int16_t msg_value = (uint8_t)(message.data[i+1]) | ((uint8_t)message.data[i] << 8);
          //int msg_value = (uint8_t)message.data[2*i]*0xFF + (uint8_t)message.data[2*i+1];
          float value = msg_value*(6.144/(5*32768));
          Serial.print(value);
          Serial.print(" ");
          dataFile.print(value);
          dataFile.print(" ");
        }
      }
      else if ((message.identifier % 0x10) == 3){ //sensor ID 3 : TC
        Serial.print("TCs: ");
        for (int i = 0; i < message.data_length_code; i+=2) {
          int16_t msg_value = (uint8_t)(message.data[i+1]) | ((uint8_t)message.data[i] << 8);
          float value = ieee754ToFloat(msg_value);
          Serial.print(value);
          Serial.print(" ");
          dataFile.print(value);
          dataFile.print(" ");
        }
      }
    }
    Serial.println();
    dataFile.println();
    dataFile.close();
  } else {
    // Skip saving if file opening fails
    Serial.println("Error opening file");
    neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0,0); // Red
    delay(50);
    setupSdCard();
  }
}
