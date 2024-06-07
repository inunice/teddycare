// Libraries
#include <Arduino.h>
// Wifi
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// Time
// #include "time.h"

// Audio
#include "DFRobotDFPlayerMini.h"
#define FPSerial Serial1
DFRobotDFPlayerMini myDFPlayer;

// Define pins
#define SOUND_SENSOR_PIN 35

// Constants
#define SOUND_WINDOW_DURATION 3000          // Recording length for initial detection of crying
#define SOUND_THRESHOLD 3500                // Threshold for baby crying
#define SOUND_COOLDOWN 60000                // Gap between crying readings (60000 = 1 minute)

// Network credentials
// #define WIFI_SSID "DragonsDen"
// #define WIFI_PASSWORD "iotcup2024fusrodah"
#define WIFI_SSID "SILVER"
#define WIFI_PASSWORD "dy3fao123"

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

// Variables for vibration
#define BUFFER_MAX_LEN 200
typedef struct DataPoint {
    int delay; // index to insert item
    int ir_value;
    int peak; // tentative muna
    int valid;
} DataPoint;
typedef struct PointArray {
  int i;//
  DataPoint arr[BUFFER_MAX_LEN];
} PointArray;


// Vibration helpers
void init_pt_array(PointArray *buffer){
  // can also serve as clearing mechanism
    buffer->i = 0;
    for (int i = 0; i < BUFFER_MAX_LEN; i++) {
      // DataPoint new_tuple;
      buffer->arr[i].delay = 0;
      buffer->arr[i].ir_value = 0;
      buffer->arr[i].peak = 0;
      buffer->arr[i].valid = 0; // para may limit yung pag traverse natin.
    }
}
int insert_pt_array(PointArray *buffer, DataPoint item){
  buffer->arr[buffer->i] = item; 
  buffer->i = (buffer->i + 1) % BUFFER_MAX_LEN;
  if (buffer->i == 0) return 1; // full indicator
  return 0;
}

// Vibration variables for Firebase
FirebaseJson json_object;
FirebaseJsonData result; // object that keeps the deserializing result

char db_path[70]; // for the path of the data in the database
PointArray long_array;
int batch = 100;
int is_recording = 0;
int is_vibrating = 0;
int counter = 0;

// PWM an
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long rest_interval = 500;
unsigned long beating_interval = 25;

const int MOTOR_TWO_PIN = 12;
const int PWM_CHANNEL_TWO = 1; 
const int PWM_FREQ = 5000;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8; // We'll use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits 


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

  // Vibration
  init_pt_array(&long_array);
  // MOTOR_PIN_TWO
  ledcSetup(PWM_CHANNEL_TWO, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_TWO_PIN, PWM_CHANNEL_TWO);

  Serial.println("Setup complete!");
}

void loop() {

  // // initialize and get the time
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Record start time
  unsigned long startTime = millis();

  // Detect if crying for window
  while ((millis() - startTime < SOUND_WINDOW_DURATION) && (cryingCoolDown == SOUND_COOLDOWN)) {
    int soundValue = analogRead(SOUND_SENSOR_PIN);   // Read value from sensor
    // Serial.println(soundValue);
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
    }

    // Read audio to play
    Serial.println("Ready to read from the database.");
    if (Firebase.RTDB.getString(&fbdo, "/speaker/audioPlaying")) {
      currentAudio = fbdo.stringData();
      Serial.print("The current audio playing is: ");
      Serial.print(currentAudio);
      Serial.println();
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    // Read volume to play
    Serial.println("Ready to read from the database.");
    if (Firebase.RTDB.getString(&fbdo, "/speaker/volume")) {
      volume = fbdo.intData();
      Serial.print("The current volume playing is: ");
      Serial.print(volume);
      Serial.println();
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    // Find index based on audio to play
    if (currentAudio == "lullaby.wav") {
      playIndex = 3;
    } else if (currentAudio == "lullaby2.wav") {
      playIndex = 4;
    } else if (currentAudio == "lullaby3.wav") {
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
    sendIsCryingPending = true;
    isCrying = 0;
  }

  // Set volume
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
  if (myDFPlayer.available() && playIndex > 0 && lastAudio == currentAudio) {
    if (!audioFinished) {
      Serial.println("Audio finished playing.");
      audioFinished = true;
      currentAudio = "";  // Reset current audio

      // Clear the audioPlaying field in Firebase
      if (Firebase.RTDB.setString(&fbdo, "/speaker/audioPlaying", "0")) {
        Serial.println("Cleared audioPlaying field in Firebase.");
      } else {
        Serial.println("Failed to clear audioPlaying field in Firebase: " + fbdo.errorReason());
      }
    }
  }

  counter = 0; //reset the counter value for a clean life.
  if (Firebase.RTDB.getInt(&fbdo, "/heartbeat_data/is_vibrating")) {
    if (fbdo.dataType() == "int") {
      is_vibrating = fbdo.intData();
      Serial.print("The value of is_vibrating is:");
      Serial.println(is_vibrating);
    }
  } else {
    Serial.println("FAILED: " + fbdo.errorReason());  }
    
  if (is_vibrating) {
    for (counter = 0; counter < BUFFER_MAX_LEN; counter++) {
      sprintf(db_path, "/heartbeat_data/preprocessed_heartbeat/spikes/%d", counter);
      if (Firebase.RTDB.getJSON(&fbdo, db_path)) {
        const String rawData = fbdo.payload();
        if (json_object.setJsonData(rawData)) {
          DataPoint new_dp;          
          json_object.get(result, "delay"); // delay
          if (result.success) new_dp.delay = result.to<int>();        
          json_object.get(result, "ir_value"); //ir value'
          if (result.success) new_dp.ir_value = result.to<int>();          
          json_object.get(result, "peak"); //peak
          if (result.success) new_dp.peak = result.to<int>();
          new_dp.valid = 1;
          insert_pt_array(&long_array, new_dp);
        }
        Serial.print("Counter: ");
        Serial.println(counter);
        delay(50);

      } else {Serial.println("Success in fetching the JSON Object! ");break;}
    }
    // Mechanical actuation
    int index = 0;
    while(is_vibrating) {
      Serial.println("Vibrating...");
      if (long_array.arr[index].valid == 1) {
        ledcWrite(PWM_CHANNEL_TWO, long_array.arr[index].ir_value);  
        Serial.print("IR Value: ");
        Serial.println(long_array.arr[index].ir_value);        
        delay(beating_interval);
        // Rest Area       
        ledcWrite(PWM_CHANNEL_TWO, LOW);
        delay(long_array.arr[index].delay);
        index++;
      }
      else {
        break;
      }
      if (Firebase.RTDB.getInt(&fbdo, "/heartbeat_data/is_vibrating")) {
        if (fbdo.dataType() == "int") {
          is_vibrating = fbdo.intData();          
        }
      } else {
        Serial.println("FAILED: " + fbdo.errorReason());
      }
    }
  }

  // Delay between readings!
  delay(1000);
}
