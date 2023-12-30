#include <SPI.h>
#include <SD.h>
#include <driver/twai.h>

// Define CAN pins
#define CAN_TX 1
#define CAN_RX 2

// Define excess CS pins
int CSpins[4] = {21,45,47,48};
SPIClass SD_SPI(HSPI);

// Define SD card pins
#define SD_CS_PIN 45
#define SD_MOSI_PIN 8
#define SD_MISO_PIN 9
#define SD_SCK_PIN 7


void setup() {
  // Initialize Serial
  Serial.begin(921600);

  // TWAI configuration
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

  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver");
    return;
  }
  Serial.println("TWAI driver started");

  // Initialize SD Card
  for (int i=0; i<4; i++){
    digitalWrite(CSpins[i],HIGH);
    delay(10);
  }
  digitalWrite(SD_CS_PIN,LOW);
  SD_SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN,SD_SPI)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");
}

void loop() {
  twai_message_t message;

  // Check if a new message is available to read
  if (twai_receive(&message, pdMS_TO_TICKS(1000)) != ESP_OK) {
    // Skip processing if reading fails
    Serial.println("Failed to read CAN packet");
    // Open file for writing error
    File dataFile = SD.open("/can_data.txt", FILE_APPEND, true);

    // If the file is available, write to it
    if (dataFile) {
      unsigned long timestamp = millis(); // Estimated timestamp
      dataFile.print("Time: ");
      dataFile.print(timestamp);
      dataFile.print(", ID: 0xFF");
      dataFile.print(", Data: ");
      dataFile.print("Failed to read CAN packet");
      dataFile.println();
      dataFile.close();
    } else {
      // Skip saving if file opening fails
      Serial.println("Error opening file");
    }
    return;
  }

  // Print the received message
  Serial.print("Message ID: ");
  Serial.print(message.identifier, HEX);
  Serial.print(" Data: ");
  for (int i = 0; i < message.data_length_code; i++) {
    Serial.print(message.data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Open file for writing
  File dataFile = SD.open("can_data.txt", FILE_WRITE);

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
        for (int i = 0; i < message.data_length_code; i+=2) {
          int16_t msg_value = (uint16_t)message.data[2*i]*0xFF + (uint16_t)message.data[2*i+1];
          float value = msg_value*(6.144/(5*2048));
          dataFile.print(value);
          dataFile.print(" ");
        }
      } else if ((message.identifier % 0x10) == 2){ //sensor ID 2: ADS1115
        for (int i = 0; i < message.data_length_code; i+=2) {
          int16_t msg_value = (uint16_t)message.data[2*i]*0xFF + (uint16_t)message.data[2*i+1];
          float value = msg_value*(6.144/(5*32768));
          dataFile.print(value);
          dataFile.print(" ");
        }
      }
      else if ((message.identifier % 0x10) == 3){ //sensor ID 3 : TC
        for (int i = 0; i < message.data_length_code; i+=2) {
          uint16_t msg_value = (uint16_t)message.data[2*i]*0xFF + (uint16_t)message.data[2*i+1];
          float value = ieee754ToFloat(msg_value);
          dataFile.print(value);
          dataFile.print(" ");
        }
      }
    }
    dataFile.println();
    dataFile.close();
  } else {
    // Skip saving if file opening fails
    Serial.println("Error opening file");
  }
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
