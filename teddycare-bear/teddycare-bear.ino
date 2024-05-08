// Libraries
#include <Arduino.h>
// Wifi
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// Time
#include "time.h"

// Define pins
#define SOUND_SENSOR_ANALOG_PIN 35

// Constants
#define AVERAGE_DURATION 10000          // Recording length for initial detection of crying
#define MIN_FLUCTUATION 50              // Fluctuation for initial detection of crying

// Network credentials
#define WIFI_SSID "NEW"
#define WIFI_PASSWORD "darksideofthedarkvader"

// Firebase credentials
#define API_KEY ""
#define DATABASE_URL "https://teddycare-12aaf-default-rtdb.asia-southeast1.firebasedatabase.app" 

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Firebase variables
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Time
const char* ntpServer = "time.nist.gov";
const long  gmtOffset_sec = 28800; // GMT+8
const int   daylightOffset_sec = 0;

// Variables
unsigned int isCrying = 0;
char timeStr[30];

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void getTimeString(char* timeString) {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
    
    // Format time into string with Firebase format
    strftime(timeString, 30, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
}

void setup() {
  
  // Initialize serial communication
  Serial.begin(115200);

  // Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }

  // Printing for Wi-Fi connection
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Accessing Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Firebase connection
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Pins for sensors and output
  pinMode(SOUND_SENSOR_ANALOG_PIN, INPUT);

  Serial.println("Setup complete!");
}

void loop() {


  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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

  // dummy
  isCrying = 1;

  // Firebase
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();    // send every 5 seconds

    // Store the flag for crying to database
    if (Firebase.RTDB.setInt(&fbdo, "soundSensor/isCrying", isCrying)) {
      Serial.println();
      Serial.print(isCrying);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ") ");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    // Store the time of crying to database
    getTimeString(timeStr);
    if (Firebase.RTDB.setString(&fbdo, "soundSensor/startTime", timeStr)) {
      Serial.println();
      Serial.print(timeStr);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ") ");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }


  // Delay between readings!
  delay(1000);
}
