#include <Arduino.h>

#define SOUND_SENSOR_PIN 35
#define SOUND_THRESHOLD 3500

void setup() {
  Serial.begin(9600);
  pinMode(SOUND_SENSOR_PIN, INPUT);
}

void loop() {
  // Read the analog value from the sound sensor
  int soundValue = analogRead(SOUND_SENSOR_PIN);
  Serial.println(soundValue);

  if (soundValue > SOUND_THRESHOLD) {
    Serial.println("baby uwu cry");
  }
  delay(100);
}
