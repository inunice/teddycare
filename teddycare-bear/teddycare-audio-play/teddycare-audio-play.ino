// Libraries
#include <Arduino.h>
// Wifi
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// // Time
// #include "time.h"

// Audio
#include "DFRobotDFPlayerMini.h"
#define FPSerial Serial1
DFRobotDFPlayerMini myDFPlayer;

// Network credentials
#define WIFI_SSID "NEW"
#define WIFI_PASSWORD "darksideofthedarkvader"

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

// Variables
String currentAudio = "";
String lastAudio = "";
unsigned int playIndex = -1;

void setup() {
  // put your setup code here, to run once:

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

  Serial.println("Setup complete!");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
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

  // Play new audio if it's different from the last audio played
  if (currentAudio != lastAudio) {
    if (playIndex > 0) {
      Serial.print("Playing audio index: ");
      Serial.println(playIndex);
      myDFPlayer.play(playIndex);  // Play file
    }
    lastAudio = currentAudio;
  }


  delay(1000);  // Delay between readings
}