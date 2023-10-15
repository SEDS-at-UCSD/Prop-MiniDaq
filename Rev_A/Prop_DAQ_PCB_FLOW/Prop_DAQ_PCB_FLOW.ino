#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"

// Task handles
TaskHandle_t transmitTaskHandle;

// Function prototypes
void transmitTask(void *pvParameters);
void flowmeterInterrupt();

// CAN TWAI message to send
twai_message_t txMessage;

// Pins used to connect to CAN bus transceiver:
#define CAN_RX 10
#define CAN_TX 9

// Pin used to connect to flowmeter (Pin 11)
#define FLOWMETER_PIN 11

// Pins used to connect to Serial bus:
#define TX2 19
#define RX2 8

const int baudrate = 921600;
const int readingInterval = 100;

volatile unsigned int pulseCount = 0;

void setup() {
  Serial.begin(baudrate); // Initialize Serial communication
  Serial2.begin(baudrate, SERIAL_8N1, RX2, TX2);

  pinMode(FLOWMETER_PIN, INPUT_PULLDOWN); // Set up pin with pull-up resistor

  // Attach an interrupt to the flowmeter pin
  //attachInterrupt(digitalPinToInterrupt(FLOWMETER_PIN), flowmeterInterrupt,RISING);
  // Create tasks
  xTaskCreatePinnedToCore(transmitTask, "TransmitTask", 4096, NULL, 1, &transmitTaskHandle, 0);
}

void loop() {
  // This is not used in FreeRTOS, so it can be left empty
  //delayMicroseconds(1);
  Serial.println();
  Serial.print("Pulse Count: ");
  Serial.println(pulseCount);
  delay(1000);
  yield();
}

void transmitTask(void *pvParameters) {
  int prev = 0;
  int curr = 0;
  int new_ = 0;
  int timecount = 0;
  int pulses = 0; 
  while (1) {
    
    
    // Read the pulse count from the flowmeter
    new_ = analogRead(FLOWMETER_PIN);
    Serial.printf("\t %d", new_);
    if ((prev < curr) and (new_ < curr)){ //every local max
      pulses++;
    }
    delayMicroseconds(1);

    if (timecount == 1000000/readingInterval){
      pulseCount = pulses*1000/readingInterval;
      //Serial.println();
      //Serial.print("Pulse Count: ");
      //Serial.println(pulseCount);
      delay(100);
      timecount = 0;
      pulses = 0;
      yield();
    }
    prev = curr;
    curr = new_;
    timecount++;
  }
}


void flowmeterInterrupt() {
  // This function is called whenever a rising edge is detected on the flowmeter pin
  pulseCount++;
}
