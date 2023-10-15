// Written by Darell Chua
//this runs at 1kHz but only 0-3.3V
int pins[4] = { 15, 16, 17, 18 };

void setup() {
  // put your setup code here, to run once:
  Serial.begin(921600);
  for (int i = 0; i < 4; i++) {
    pinMode(pins[i], INPUT_PULLDOWN);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Pin ");
  Serial.print(pins[0]);
  Serial.print(": ");
  Serial.print(3.3*(float)analogRead(pins[0])/4095,4);
  for (int i = 1; i < 4; i++) {
    Serial.print("\tPin ");
    Serial.print(pins[i]);
    Serial.print(": ");
      Serial.print(3.3*(float)analogRead(pins[i])/4095,4);
  }
  Serial.println();
  delay(1);
}
