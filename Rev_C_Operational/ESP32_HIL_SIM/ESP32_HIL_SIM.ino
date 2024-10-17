#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>  // For random data generation

// Simulated CAN Message Structure
struct SimulatedCANMessage {
  int identifier;
  int data_length_code;
  uint8_t data[8];
};

volatile int vari_able = 0;

// Function to simulate CAN data generation for a specific board
SimulatedCANMessage generateSimulatedCANMessage(int boardID) {
  SimulatedCANMessage message;
  message.identifier = (0x10 * boardID);  // Unique identifier for each board
  message.data_length_code = 8;  // Standard data length for CAN messages

  // Simulate sensor data
  for (int i = 0; i < 8; i++) {
    if ((i % 2) == 0) {
      message.data[i] = (vari_able/256);  // for positivity: needs to be within int16 LOL, so 127Random byte between 0-255 
    } else {
      message.data[i] = (vari_able/256) - (vari_able/256)*256 ;  // needs to be within int16 LOL, so 127Random byte between 0-255
    }
  }

  if (vari_able > 26665) //32767
    vari_able = 0;
  else
    ++ vari_able;
  return message;
}

// Function to process and print the simulated CAN message as if it's read from the CAN bus
void processSimulatedCANMessage(SimulatedCANMessage message) {
  StaticJsonDocument<256> jsonData;

  // Simulate extracting board ID and sensor type
  int boardID = (message.identifier/0x10); //(message.identifier / 0x10);
  jsonData["BoardID"] = String(message.identifier + boardID,HEX);

  // Fake different sensor types for demonstration
  jsonData["SensorType"] = String(boardID,HEX); //for demo match with ID


  // Simulate processing the CAN message data
  for (int i = 0; i < message.data_length_code; i++) {
    jsonData["Sensors"][i] = message.data[i];
  }

  // Serialize the JSON and print it to simulate output
  serializeJson(jsonData, Serial);
  Serial.println();
}

// Task to simulate receiving and processing CAN messages
void canReaderTask(void *pvParameters) {
  while (1) {
    // Simulate receiving CAN messages from 3 different boards
    for (int boardID = 1; boardID <= 4; boardID++) {
      SimulatedCANMessage message = generateSimulatedCANMessage(boardID);
      processSimulatedCANMessage(message);
    }

    // Simulate delay between receiving messages
    //vTaskDelay(1 / portTICK_PERIOD_MS);
    yield();
  }
}

void setup() {
  Serial.begin(921600);

  // Start the simulated CAN reader task
  xTaskCreatePinnedToCore(canReaderTask, "CANReaderTask", 4096, NULL, 1, NULL, 1);

  Serial.println("Simulated CAN Reader Hub started...");
}

void loop() {
  // Main loop does nothing, as the CAN reader is running in a separate task
  delay(1000);
}
