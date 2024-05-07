// Libraries
#include <Arduino.h>

// Define pins
#define SOUND_SENSOR_ANALOG_PIN 35

// Constants
#define AVERAGE_DURATION 10000          // Recording length for initial detection of crying
#define MIN_FLUCTUATION 50              // Fluctuation for initial detection of crying

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Setup complete!");
}

void loop() {
  // Variables to store cumulative sum and count of readings
  unsigned long sum = 0;
  unsigned int count = 0;

  // Record start time
  unsigned long startTime = millis();

  // Loop to get data until the duration is reached
  while (millis() - startTime < AVERAGE_DURATION) {
    int soundValue = analogRead(SOUND_SENSOR_ANALOG_PIN);   // Read value from sensor
    sum += soundValue;      // Add reading to sum
    count++;
    delay(10);    // Delay between readings
  }

  // Average for the duration
  int average = sum / count;

  // Monitoring
  Serial.print("Average sound sensor value: ");
  Serial.println(average);

  // Variable to track the number of significant fluctuations
  int fluctuationCount = 0;

  // Loop again to detect fluctuations
  startTime = millis(); // Reset start time
  while (millis() - startTime < AVERAGE_DURATION) {
    int soundValue = analogRead(SOUND_SENSOR_ANALOG_PIN);   // Read value from sensor
    // Check if the absolute difference from the average exceeds the minimum fluctuation
    if (abs(soundValue - average) >= MIN_FLUCTUATION) {
      fluctuationCount++;
    }
    delay(10);    // Delay between readings
  }

  // Check if there a significant number of fluctations
  if (fluctuationCount > 5) { // 
    Serial.println("Baby crying detected!");
  }

  // Delay between readings!
  delay(1000);
}
