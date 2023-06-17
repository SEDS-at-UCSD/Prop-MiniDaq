#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ADS1X15.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

TwoWire I2C_one = TwoWire(0);
// Initialize the OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_one, OLED_RESET);

ADS1115 ADS(0x48,&I2C_one);


volatile int CPS = 0; //instructions per second
volatile int prevCPS;

volatile int flowPulses = 0;   // Keeps track of the number of pulses from the flow sensor
volatile float flowRate;       // Stores the calculated flow rate

QueueHandle_t flowRateQueue;   // Queue handler to communicate flow rate values between tasks
volatile int usedisplay = 0; //false for speed
volatile int get_INA219 = 0;
volatile int get_ADS1115 = 1;

volatile int switch2InternalADC_12 = 0; //true use internal
const uint8_t intDAQ0 = 15;
const uint8_t intDAQ1 = 17;
const int Solenoid0 = 16;
const int Solenoid1 = 45; //arbitrary non-use pin because not in use, typical: 18
// Define the input pin for the flow sensor
const int flowSensorPin = 18;

volatile bool Solenoid0Status = false;
volatile bool Solenoid1Status = false;
volatile bool prevButton = false;

const int JoyY = 4;
const int JoyBtn = 5;
const int JoyPower = 12;



void serialTask(void *parameter) {
  while (1) {
    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      if (command.startsWith("Sol0on")) {
        Solenoid0Status = 1;
      }else if (command.startsWith("Sol0off")) {
        Solenoid0Status = 0;
      }
      if (command.startsWith("Sol1on")) {
        Solenoid1Status = 1;
      }else if (command.startsWith("Sol1off")) {
        Solenoid1Status = 0;
      }
      if (command.startsWith("displayon")) {
        usedisplay = 1;
      }else if (command.startsWith("displayoff")) {
        usedisplay = 0;
      }
      if (command.startsWith("INA219on")) {
        get_INA219 = 1;
      }else if (command.startsWith("INA219off")) {
        get_INA219 = 0;
      }
      if (command.startsWith("ADS1115on")) {
        get_ADS1115 = 1;
      }else if (command.startsWith("ADS1115off")) {
        get_ADS1115 = 0;
      }
      if (command.startsWith("ADCInternal_12")) { //pins 15 and 17
        switch2InternalADC_12 = 1;
      }else if (command.startsWith("ADCExternal_12")) { //ADS pins 1 and 2
        switch2InternalADC_12 = 0;
      }
    }
    vTaskDelay(1); // Delay to avoid hogging the CPU
  }
}

void flowRateManual(void *parameter)  { //replaces interrupt
  //pinMode(flowSensorPin, INPUT_PULLDOWN);  // Set the flow sensor pin as an input and enable the internal pull-up resistor
  int lowthresh = 5;
  int highthresh = 45;
  while(1){
    static int prevLL = 0;
    int currLL = 0;
    int milliVVal = analogReadMilliVolts(flowSensorPin);
    if(milliVVal > highthresh){
      currLL = 1;
      prevLL = 0;
    }
    if(milliVVal < lowthresh){
      prevLL = currLL;
      currLL = 0;
    }
    if((prevLL - currLL) == 1){ //falling edge
      flowSensorISR();//flowPulses++;
    }
    yield();
    milliVVal = analogReadMilliVolts(flowSensorPin);
    if(milliVVal > highthresh){
      currLL = 1;
      prevLL = 0;
    }
    if(milliVVal < lowthresh){
      prevLL = currLL;
      currLL = 0;
    }
    if((prevLL - currLL) == 1){ //falling edge
      flowSensorISR();//flowPulses++;
    }
    yield();
    milliVVal = analogReadMilliVolts(flowSensorPin);
    if(milliVVal > highthresh){
      currLL = 1;
      prevLL = 0;
    }
    if(milliVVal < lowthresh){
      prevLL = currLL;
      currLL = 0;
    }
    if((prevLL - currLL) == 1){ //falling edge
      flowSensorISR();//flowPulses++;
    }
    yield();
    milliVVal = analogReadMilliVolts(flowSensorPin);
    if(milliVVal > highthresh){
      currLL = 1;
      prevLL = 0;
    }
    if(milliVVal < lowthresh){
      prevLL = currLL;
      currLL = 0;
    }
    if((prevLL - currLL) == 1){ //falling edge
      flowSensorISR();//flowPulses++;
    }
    yield();
    milliVVal = analogReadMilliVolts(flowSensorPin);
    if(milliVVal > highthresh){
      currLL = 1;
      prevLL = 0;
    }
    if(milliVVal < lowthresh){
      prevLL = currLL;
      currLL = 0;
    }
    if((prevLL - currLL) == 1){ //falling edge
      flowSensorISR();//flowPulses++;
    }
    yield();
    milliVVal = analogReadMilliVolts(flowSensorPin);
    if(milliVVal > highthresh){
      currLL = 1;
      prevLL = 0;
    }
    if(milliVVal < lowthresh){
      prevLL = currLL;
      currLL = 0;
    }
    if((prevLL - currLL) == 1){ //falling edge
      flowSensorISR();//flowPulses++;
    }
    yield();
    
    vTaskDelay(1);
  }
}

void flowRateTask(void *pvParameters) {
  (void) pvParameters;

  //pinMode(flowSensorPin, INPUT_PULLUP);  // Set the flow sensor pin as an input and enable the internal pull-up resistor
  //pinMode(flowSensorPin, INPUT_PULLDOWN);
  //attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowSensorISR, FALLING);  // Attach an interrupt to the flow sensor pin to count pulses

  for (;;) {
    CPS = 0;
    flowPulses = 0;             // Reset the pulse count
    int delay_num = 1000; //wait for 1 second
    vTaskDelay(pdMS_TO_TICKS(delay_num));  // Wait for one second to allow pulses to accumulate
    //detachInterrupt(digitalPinToInterrupt(flowSensorPin));  // Disable the interrupt while calculating the flow rate
    float flowRate = (1000/(float)delay_num) * (float)flowPulses;  // Calculate the flow rate in liters per minute, assuming a flow sensor with a pulse rate of 7.5 pulses per liter
    xQueueSend(flowRateQueue, &flowRate, portMAX_DELAY);  // Send the flow rate value to the queue
    //attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowSensorISR, FALLING);  // Re-enable the interrupt to count pulses
  }
}

void setup() 
{
  pinMode(flowSensorPin, INPUT_PULLUP); //pinMode(flowSensorPin, INPUT_PULLUP);

  Serial.begin(921600);
  I2C_one.begin(13,14);
  I2C_one.setClock(400000);

  //Power Monitoring
  /*
  if (! ina219.begin(&I2C_one)) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  */
  
  // OLED display initialization
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  //display.clearDisplay();


  display.clearDisplay();
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("SEDS");
  display.display();


  Serial.println(__FILE__);
  Serial.print("ADS1X15_LIB_VERSION: ");
  Serial.println(ADS1X15_LIB_VERSION);
  ADS.begin();
  ADS.setWireClock(400000);
  ADS.setDataRate(7);
  //ADS.writeRegister(ADS1015_REG_CONFIG, config | ADS1015_REG_CONFIG_DR_860SPS);

  delay(750);
  display.clearDisplay();
  display.setTextSize(5);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("AT");
  display.display();

  delay(500);
  display.clearDisplay();
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("UCSD");
  display.display();

  pinMode(0,INPUT_PULLUP);
  pinMode(Solenoid0,OUTPUT);
  pinMode(Solenoid1,OUTPUT);

  pinMode(JoyPower,OUTPUT);
  pinMode(JoyY,INPUT_PULLDOWN);
  pinMode(JoyBtn,INPUT_PULLUP);

  delay(750);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Propulsion");
  display.println("Team DAQ");
  display.display();

  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("SEDS AT UCSD");
  display.setCursor(0, 10);
  display.print("Propulsion DAQ");
  display.setCursor(0, 20);
  display.print("Beta v0.2");
  display.setCursor(0, 30);
  display.print("Developed by:");
  display.setCursor(0, 40);
  display.print("Darell Chua");
  display.setCursor(0, 50);
  display.print("Updated 26 May 2023");
  display.display();

  xTaskCreatePinnedToCore(
    serialTask,       // Task function
    "SerialTask",     // Task name
    4096,             // Stack size (adjust as needed)
    NULL,             // Task parameter
    1,                // Priority (adjust as needed)
    NULL,             // Task handle
    0                 // Core to run the task on (0 or 1)
  );

  flowRateQueue = xQueueCreate(1, sizeof(float));  // Create a queue to hold one float value
  xTaskCreate(flowRateTask, "FlowRateTask", configMINIMAL_STACK_SIZE + 1024, NULL, 1, NULL);
  //xTaskCreatePinnedToCore(flowRateManual, "FlowRateManual", 4096, NULL, 1, NULL,0);

  analogReadResolution(12); // Set ADC resolution to 12 bits
  pinMode(intDAQ0,INPUT_PULLUP);
  pinMode(intDAQ1,INPUT_PULLUP);
  delay(1000);



}


void loop() 
{ 
  static int currCPS = 0;
  static float lastFlowRate = 0;
  float latestFlowRate;
  if (xQueueReceive(flowRateQueue, &latestFlowRate, 0) == pdPASS) {  // Try to read the latest flow rate value from the queue
    lastFlowRate = latestFlowRate;
  }
  static int solcase = -1;
  static bool firsttime = true;


  if (usedisplay){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
  }
  if (get_ADS1115){
    ADS.setGain(0);

    int16_t val_0, val_1;
    if(switch2InternalADC_12){
      val_0 = analogRead(intDAQ0);
      val_1 = analogRead(intDAQ1);
    }
    else{
      val_0 = ADS.readADC(0);  
      val_1 = ADS.readADC(1);  
    }
    int16_t val_2 = ADS.readADC(2);  
    int16_t val_3 = ADS.readADC(3);  

    float f = ADS.toVoltage(1);  // voltage factor

    float V_val_0  = val_0 * f ;
    float V_val_1  = val_1 * f ;
    float V_val_2  = val_2 * f ;
    float V_val_3  = val_3 * f ;

    if(switch2InternalADC_12){
      V_val_0 = (val_0*3.3/4095.0);
      V_val_1 = (val_1*3.3/4095.0);
    } 
    
    Serial.print("Analog0: "); Serial.print(V_val_0, 6); Serial.print('\t'); 
    Serial.print("Analog1: "); Serial.print(V_val_1, 6); Serial.print('\t'); 

    
    Serial.print("Analog2: "); Serial.print(V_val_2, 6); Serial.print('\t');
    Serial.print("Analog3: "); Serial.print(V_val_3, 6);

    /*
    Serial.print("Analog0: "); Serial.print(val_0); Serial.print('\t'); Serial.println(val_0 * f, 3);
    Serial.print("Analog1: "); Serial.print(val_1); Serial.print('\t'); Serial.println(val_1 * f, 3);
    Serial.print("Analog2: "); Serial.print(val_2); Serial.print('\t'); Serial.println(val_2 * f, 3);
    Serial.print("Analog3: "); Serial.print(val_3); Serial.print('\t'); Serial.println(val_3 * f, 3);
    */
    //Serial.println();


    if (usedisplay){

      display.setCursor(0, 0);
      display.print("Analog0:");
      display.setCursor(50, 0);
      display.print(V_val_0, 3);
      display.print("V");

      display.setCursor(0, 10);
      display.print("Analog1:");
      display.setCursor(50, 10);
      display.print(V_val_1, 3);
      display.print("V");

      display.setCursor(0, 20);
      display.print("Analog2:");
      display.setCursor(50, 20);
      display.print(V_val_2, 3);
      display.print("V");

      display.setCursor(0, 30);
      display.print("Analog3:");
      display.setCursor(50, 30);
      display.print(V_val_3, 3);
      display.print("V");
    }
  }

  
  // Display the readings on the OLED display
  
  

  bool SolenoidSwitching = false;
  int ButtonRead = digitalRead(JoyBtn);
  if ((ButtonRead - prevButton)>0){
    SolenoidSwitching = true; //only rising edge trigger
  }

  if(firsttime){
    SolenoidSwitching = false;
    firsttime = false;
  }
  prevButton = ButtonRead;
  int JoyRead = analogRead(JoyY);

  if (JoyRead > 3500){
    solcase = 0;
  } else if (JoyRead < 1500){
    solcase = 1;
  }

  if (solcase == 0){
    if (usedisplay){
      display.setCursor(0, 40);
      display.print("X");
    }
    if (SolenoidSwitching){
      Solenoid0Status = !Solenoid0Status;
    }
  } else if (solcase == 1) {
    if (usedisplay){
      display.setCursor(00, 50);
      display.print("X");
    }
    if (SolenoidSwitching){
      Solenoid1Status = !Solenoid1Status;
    }
  }

  display.setCursor(10, 40);
  display.print("Sol 0:");
  display.setCursor(50, 40);

  if(Solenoid0Status){
    digitalWrite(Solenoid0,HIGH);
    if (usedisplay){
      display.print("on");
    }
  }else{
    digitalWrite(Solenoid0,LOW);
    if (usedisplay){
      display.print("off");
    }
  }

  Serial.print("\tSol0: ");
  Serial.print(Solenoid0Status);

  if (usedisplay){display.setCursor(10, 50);}
  if (usedisplay){display.print("Sol 1:");}
  if (usedisplay){display.setCursor(50, 50);}

  if(Solenoid1Status){
    digitalWrite(Solenoid1,HIGH);
    display.print("on");
  }else{
    digitalWrite(Solenoid1,LOW);
    display.print("off");
  }

  Serial.print("\tSol1: ");
  Serial.print(Solenoid1Status);

  analogWrite(JoyPower,1023);
  Serial.print("\tJoystick_UD: ");
  Serial.print(JoyRead);
  Serial.print("\tJoystick_Btn: ");
  Serial.print(ButtonRead);

  

  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
  
  if (get_INA219){
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
    
    Serial.print("\tBus Voltage (V): "); Serial.print(busvoltage);
    Serial.print("\tShunt Voltage (mV): "); Serial.print(shuntvoltage);
    Serial.print("\tLoad Voltage (V): "); Serial.print(loadvoltage);
    Serial.print("\tCurrent (mA): "); Serial.print(current_mA);
    Serial.print("\tPower (mW): "); Serial.print(power_mW);

    if (usedisplay){
      display.setCursor(90, 0);
      display.print("|Flow:");
      display.setCursor(90, 10);
      display.print("|");
      display.print(lastFlowRate);
      display.setCursor(90, 20);
      display.print("|PPS");
    }
    
  }

  Serial.print("\tFlow (pulses per second): ");
  Serial.print(lastFlowRate);

  if(CPS == 0){
    currCPS = prevCPS;
  }
  Serial.print("\tCPS: ");
  Serial.print(currCPS);
  prevCPS = CPS;
  CPS++;

  Serial.println();

  if (usedisplay){
    display.display();
  }
  //delay(1);
  //delayMicroseconds(500);
}

void flowSensorISR() {
  flowPulses++;  // Increment the pulse count
}


// -- END OF FILE --

