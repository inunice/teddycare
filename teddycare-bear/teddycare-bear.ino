// Libraries
#include <Arduino.h>
// Wifi
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// Time
#include "time.h"

// Audio
#include "DFRobotDFPlayerMini.h"
#define FPSerial Serial1
DFRobotDFPlayerMini myDFPlayer;

// Define pins
#define SOUND_SENSOR_PIN 35

// Constants
#define SOUND_WINDOW_DURATION 5000          // Recording length for initial detection of crying
#define SOUND_THRESHOLD 3500                // Threshold for baby crying
#define SOUND_COOLDOWN 60000                // Gap between crying readings (60000 = 1 minute)

// Network credentials
#define WIFI_SSID "DragonsDen"
#define WIFI_PASSWORD "iotcup2024fusrodah"

// Firebase credentials
#define API_KEY "AIzaSyCYMkG_fXoxCRsKImpuWSHvSOZq_zv1fJU"
#define DATABASE_URL "https://teddycare-12aaf-default-rtdb.asia-southeast1.firebasedatabase.app" 

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Firebase variables
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Pending variables to retry failed sends to Firebase
bool sendIsCryingPending = true;
bool sendStartTimePending = true;
bool clearAudioPlayingPending = true;

// Time
const char* ntpServer = "time.nist.gov";
const long  gmtOffset_sec = 28800; // GMT+8
const int   daylightOffset_sec = 0;

// Variables for sound sensor
unsigned int isCrying = 0;
unsigned int cryingCoolDown = SOUND_COOLDOWN;
char timeStr[30];

// Variables for playing audio
String currentAudio = "";
String lastAudio = "None";
unsigned int playIndex = -1;
bool audioFinished = false;
unsigned int volume = 15;

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
  
  // Initialize sound
  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/27, /*tx =*/26);   // Start sound player

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

  // Sound
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0);
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  // Pins for sensors and output
  pinMode(SOUND_SENSOR_PIN, INPUT);

  Serial.println("Setup complete!");
}

void loop() {

  // initialize and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Record start time
  unsigned long startTime = millis();

  // Detect if crying for 5 seconds
  while ((millis() - startTime < SOUND_WINDOW_DURATION) && (cryingCoolDown == SOUND_COOLDOWN)) {
    int soundValue = analogRead(SOUND_SENSOR_PIN);   // Read value from sensor
    Serial.println(soundValue);
    if (soundValue > SOUND_THRESHOLD) {
      isCrying = 1;
      Serial.println("Baby is crying!");
      break;
    }
    delay(10);    // Delay between readings
  }

  // // Check sound values
  // Serial.print("Cooldown");
  // Serial.println(cryingCoolDown);
  // Serial.print("Crying?");
  // Serial.println(isCrying);

  // Firebase
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    // Store to database ONLY if crying is 1 and not pending
    if ((isCrying == 1)) {
      sendDataPrevMillis = millis();    // send every second
      Serial.println("Ready to send to database.");
    
      // Send isCrying status
      if (sendIsCryingPending) {
        if (Firebase.RTDB.setInt(&fbdo, "/soundSensor/isCrying", isCrying)) {
          Serial.println();
          Serial.print(isCrying);
          Serial.print(" - successfully saved to: " + fbdo.dataPath());
          Serial.println(" (" + fbdo.dataType() + ") ");
          sendIsCryingPending = false;
        } else {
          Serial.println("FAILED: " + fbdo.errorReason());
          sendIsCryingPending = true;
        }
      }

      // Send start time
      if (sendStartTimePending) {
        getTimeString(timeStr);
        if (Firebase.RTDB.setString(&fbdo, "/soundSensor/startTime", timeStr)) {
          Serial.println();
          Serial.print(timeStr);
          Serial.print(" - successfully saved to: " + fbdo.dataPath());
          Serial.println(" (" + fbdo.dataType() + ") ");
          sendStartTimePending = false;
        } else {
          Serial.println("FAILED: " + fbdo.errorReason());
          sendStartTimePending = true;
        }
      }
    }

    // Read audio to play
    Serial.println("Ready to read from the database.");
    if (Firebase.RTDB.getString(&fbdo, "/speaker/audioPlaying")) {
      if (fbdo.dataType() == "string") {
        currentAudio = fbdo.stringData();
        Serial.print("The current audio playing is: ");
        Serial.print(currentAudio);
        Serial.println();
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    // Read audio to play
    Serial.println("Ready to read from the database.");
    if (Firebase.RTDB.getString(&fbdo, "/speaker/volume")) {
      if (fbdo.dataType() == "int") {
        volume = fbdo.intData();
        Serial.print("The current volume playing is: ");
        Serial.print(volume);
        Serial.println();
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    // Find index based on audio to play
    if (currentAudio == "gs://teddycare-12aaf.appspot.com/lullaby.wav") {
      playIndex = 3;
    } else if (currentAudio == "gs://teddycare-12aaf.appspot.com/lullaby2.wav") {
      playIndex = 4;
    } else if (currentAudio == "gs://teddycare-12aaf.appspot.com/lullaby3.wav") {
      playIndex = 5;
    } else {
      playIndex = 0; // No audio playing!
    }
  }

  // Decrease cooldown if baby is crying
  if (isCrying == 1) {
    Serial.print("Crying detection on cooldown; seconds left: ");
    Serial.println(cryingCoolDown / 1000); // Print seconds left by dividing milliseconds by 1000
    cryingCoolDown = cryingCoolDown - 1000;
  }

  // If cooldown is done, reset cooldown!
  if (cryingCoolDown == 0) {
    Serial.println("Cooldown is done! We will enable sound detection again.");
    cryingCoolDown = SOUND_COOLDOWN;
    isCrying = 0;
  }

  myDFPlayer.volume(volume);
  
  // Play audio
  if (currentAudio != lastAudio) {  // Play if different from last audio playing
    if (playIndex > 0) {
      Serial.print("Playing audio index: ");
      Serial.println(playIndex);
      myDFPlayer.play(playIndex);  // Play file
      audioFinished = false;  // Reset audio finished flag
    }
    lastAudio = currentAudio;
  }

  // Check if audio finished playing
  if (myDFPlayer.available() && playIndex > 0) {
    if (!audioFinished) {
      Serial.println("Audio finished playing.");
      audioFinished = true;
      currentAudio = "";  // Reset current audio

      // Clear the audioPlaying field in Firebase
      if (clearAudioPlayingPending) {
        if (Firebase.RTDB.setString(&fbdo, "/speaker/audioPlaying", "0")) {
          Serial.println("Cleared audioPlaying field in Firebase.");
          clearAudioPlayingPending = false;
        } else {
          Serial.println("Failed to clear audioPlaying field in Firebase: " + fbdo.errorReason());
          clearAudioPlayingPending = true;
        }
      }
    }
  }

  // Delay between readings!
  delay(1000);
}
