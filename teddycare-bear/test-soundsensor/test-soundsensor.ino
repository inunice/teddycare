#include <Arduino.h>

#define SOUND_SENSOR_PIN 35

void setup() {
  Serial.begin(9600);
  pinMode(SOUND_SENSOR_PIN, INPUT);
}

void loop() {
  // Read the analog value from the sound sensor
  int sensorValue = analogRead(SOUND_SENSOR_PIN);
  Serial.println(sensorValue);
  delay(100);
}
