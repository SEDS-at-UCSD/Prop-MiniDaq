#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ADS1X15.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


TwoWire I2C_one = TwoWire(0);
// Initialize the OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_one, OLED_RESET);

ADS1115 ADS(0x48,&I2C_one);

const int Solenoid0 = 16;
const int Solenoid1 = 18;

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
    }
    vTaskDelay(1); // Delay to avoid hogging the CPU
  }
}

void setup() 
{
  Serial.begin(115200);

    
  I2C_one.begin(13,14);
  I2C_one.setClock(400000);
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

  delay(1000);
  
}


void loop() 
{
  static int solcase = 0;
  static bool firsttime = true;
  ADS.setGain(0);

  int16_t val_0 = ADS.readADC(0);  
  int16_t val_1 = ADS.readADC(1);  
  int16_t val_2 = ADS.readADC(2);  
  int16_t val_3 = ADS.readADC(3);  

  float f = ADS.toVoltage(1);  // voltage factor

  /*
  Serial.print("Analog0: "); Serial.print(val_0); Serial.print('\t'); Serial.println(val_0 * f, 3);
  Serial.print("Analog1: "); Serial.print(val_1); Serial.print('\t'); Serial.println(val_1 * f, 3);
  Serial.print("Analog2: "); Serial.print(val_2); Serial.print('\t'); Serial.println(val_2 * f, 3);
  Serial.print("Analog3: "); Serial.print(val_3); Serial.print('\t'); Serial.println(val_3 * f, 3);
  */
  //Serial.println();

  Serial.print("Analog0: "); Serial.print(val_0 * f, 6); Serial.print('\t'); 
  Serial.print("Analog1: "); Serial.print(val_1 * f, 6); Serial.print('\t');
  Serial.print("Analog2: "); Serial.print(val_2 * f, 6); Serial.print('\t');
  Serial.print("Analog3: "); Serial.print(val_3 * f, 6);

  float V_val_0  = val_0 * f ;
  float V_val_1  = val_1 * f ;
  float V_val_2  = val_2 * f ;
  float V_val_3  = val_3 * f ;

  // Display the readings on the OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("Analog0:");
  display.setCursor(60, 0);
  display.print(V_val_0, 3);
  display.print("V");

  display.setCursor(0, 10);
  display.print("Analog1:");
  display.setCursor(60, 10);
  display.print(V_val_1, 3);
  display.print("V");

  display.setCursor(0, 20);
  display.print("Analog2:");
  display.setCursor(60, 20);
  display.print(V_val_2, 3);
  display.print("V");

  display.setCursor(0, 30);
  display.print("Analog3:");
  display.setCursor(60, 30);
  display.print(V_val_3, 3);
  display.print("V");

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
    display.setCursor(0, 40);
    display.print("X");
    if (SolenoidSwitching){
      Solenoid0Status = !Solenoid0Status;
    }
  } else if (solcase == 1) {
    display.setCursor(00, 50);
    display.print("X");
    if (SolenoidSwitching){
      Solenoid1Status = !Solenoid1Status;
    }
  }

  display.setCursor(10, 40);
  display.print("Sol 0:");
  display.setCursor(60, 40);

  if(Solenoid0Status){
    digitalWrite(Solenoid0,HIGH);
    display.print("on");
  }else{
    digitalWrite(Solenoid0,LOW);
    display.print("off");
  }

  Serial.print("\tSol0: ");
  Serial.print(Solenoid0Status);

  display.setCursor(10, 50);
  display.print("Sol 1:");
  display.setCursor(60, 50);

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

  Serial.println();
  display.display();
  delayMicroseconds(500);
}


// -- END OF FILE --

