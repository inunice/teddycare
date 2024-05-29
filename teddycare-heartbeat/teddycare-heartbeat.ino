// Libraries
#include <Arduino.h>
// #include <Wire.h>
#include "MAX30105.h"
// #include <stdio.h>
// Wifi
#include <ESP8266WiFi.h>
// #include <WiFiClient.h>
// #include <WiFi.h>
// Firebase
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// Time
#include "time.h"

// Network credentials
// #define WIFI_SSID "GlobeAtHome_8A984_2.4"
// #define WIFI_PASSWORD "MCm7fGGY"
#define WIFI_SSID "SILVER"
#define WIFI_PASSWORD "dy3fao123"
// #define WIFI_SSID "DragonsDen"
// #define WIFI_PASSWORD "iotcup2024fusrodah"
// #define WIFI_SSID "dcs-students2"
// #define WIFI_PASSWORD "W1F14students"
// #define WIFI_SSID "ENGG-Student-WiFi"
// #define WIFI_PASSWORD "c03l1br@ry"

// Firebase credentials
// #define API_KEY "AIzaSyC21Lyo6PDNBpShPR1b8PZ2HreeaTwRpa0"
// #define DATABASE_URL "https://test1-a4e94-default-rtdb.asia-southeast1.firebasedatabase.app/" 
#define API_KEY "AIzaSyCYMkG_fXoxCRsKImpuWSHvSOZq_zv1fJU"
#define DATABASE_URL "https://teddycare-12aaf-default-rtdb.asia-southeast1.firebasedatabase.app/" 
// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Firebase variables
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Tuple struct
typedef struct tuple {
  int from_start_device_time;
  int ir_value;
} tuple;

// Ali Jafri's Ring Buffer

#include <bits/stdc++.h>

#define BUFFER_MAX_LEN 20 // must be changed since malalaking values nga naman

typedef struct RingBuffer {
    int i; // index to insert item
    tuple arr[BUFFER_MAX_LEN];
} RingBuffer;

void init_ring_buffer(RingBuffer *buffer){
    buffer->i = 0;
    for (int i = 0; i < BUFFER_MAX_LEN; i++) {
      // tuple new_tuple;
      buffer->arr[i].from_start_device_time = 0;
      buffer->arr[i].ir_value = 0;
    }

}

int insert_ring_buffer(RingBuffer *buffer, tuple item){
  buffer->arr[buffer->i] = item; 
  buffer->i = (buffer->i + 1) % BUFFER_MAX_LEN;
  if (buffer->i == 0) return 1; // full indicator
  return 0;
}

// for debugging
void print_ring_buffer(RingBuffer *buffer){
    for (int i = 0; i < BUFFER_MAX_LEN; i++)
      printf("%d ", buffer->arr[i]);
    printf("\n");
}

void clear_json_array(FirebaseJsonArray *json_array){  
  json_array->clear();
}

// Data structures
RingBuffer subset;
int PULSE_RECORD_TIME_DURATION = 30000;
FirebaseJsonArray json_array;
char db_path[50]; // for the path of the data in the database
int batch = 100;
int isRecording = 0;
int is_uploading = 0;
int ledPin = D5; // Define the pin number for the LED
int isCrying = 0; // Initialize the isCrying variable

// hardware
MAX30105 particleSensor;

void setup(){
  // Initialize serial communication
  Serial.begin(115200);

  // Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
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
    Serial.println("Firebase signup ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Firebase connection
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  
  /**
  Setup Hardware components
  */

  // Pin
  pinMode(ledPin, OUTPUT); // Set the LED pin as an output

  //// Initialize sensor
  Serial.println("Setting up the heartbeat sensor...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  //Setup to sense a nice looking saw tooth on the plotter
  byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

  // Arduino plotter auto-scales annoyingly. To get around this, pre-populate
  // The plotter with 500 of an average reading from the sensor

  // Take an average of IR readings at power up

  Serial.println("Powering up the sensor...");

  const byte avgAmount = 64;
  long baseValue = 0;
  for (byte x = 0 ; x < avgAmount ; x++)
  {
    baseValue += particleSensor.getIR(); //Read the IR value
  }
  baseValue /= avgAmount;

  //Pre-populate the plotter so that the Y scale is close to IR values
  for (int x = 0 ; x < 500 ; x++)
    Serial.println(baseValue);

  Serial.println("Pulse sensor now powered up!");

  // Setup Data structures t.y. jafri!
  init_ring_buffer(&subset);  
}

void loop(){
  
  // we will only consider the values > 90,000 [determined via previous experiments]
  // Send to firebase if: 
  // the timer is over / buffer is full
  // if the buffer is sent even if the remaining elements are from the previous buffer, replace them.
  // // replace their time values from [latest time] to [latest time + k] where k is the number of slots remaining in the buffer from the previous buffer iteration
  // 

  if (Firebase.RTDB.getInt(&fbdo, "/soundSensor/isCrying/")) {
    if (fbdo.dataType() == "int") {
      isCrying = fbdo.intData();
      Serial.println("The isCrying value is " + isCrying);
    }
  }
  else {
    Serial.println("Can't Fetch isCrying " + fbdo.errorReason());
  }

  if (isCrying == 1) {
    Serial.println("Crying!");
    digitalWrite(ledPin, HIGH); // Turn the LED on
  } else {
    digitalWrite(ledPin, LOW); // Turn the LED off
  }


  if (Firebase.RTDB.getInt(&fbdo, "/heartbeat_data/is_recording/")) {
    if (fbdo.dataType() == "int") {
      isRecording = fbdo.intData();
      Serial.println("The isRecording value is " + isRecording);
    }
  }
  else {
    Serial.println("Can't Fetch isRecording " + fbdo.errorReason());
  }
  int reading = particleSensor.getIR();
  if (isRecording == 1 && reading > 90000) {
    Serial.println("Starting Recording");
    unsigned long startTime = millis();    
    int duration = 0;

    while((duration < PULSE_RECORD_TIME_DURATION)) {
      // Record the IR readings
      reading = particleSensor.getIR();
      duration = millis() - startTime;
      tuple new_tuple;
      new_tuple.from_start_device_time = duration;
      new_tuple.ir_value = reading - 90000 ; // for scaling
      Serial.println(new_tuple.ir_value);

      if(90000 < reading < 200000 && insert_ring_buffer(&subset, new_tuple) ) { 
        // send to firebase
        // print_ring_buffer(&subset);
        if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
          sendDataPrevMillis = millis();    // send every second

          // transfer contents from RingBuffer to FirebaseJsonArray
          for (int j = 0; j <= BUFFER_MAX_LEN; j++){
            FirebaseJson data_point;
            data_point.add("from_start_device_time", subset.arr[j].from_start_device_time);
            data_point.add("ir_value", subset.arr[j].ir_value);
            // Serial.print(subset.arr[j].ir_value + " ");
            json_array.add(data_point);
          }
          Serial.println();
          // push the array to the database in a PATH
          // sprintf(db_path, "/heartbeat_data/recordings/batch%d", batch);
          sprintf(db_path, "/heartbeat_data/recordings/");
          //sprintf(db_path, "/heartbeat_data/scaled/");
          if (Firebase.RTDB.pushArray(&fbdo, db_path, &json_array)) {
            Serial.println("PASSED");
            Serial.println("PATH: " + fbdo.dataPath());
            Serial.println("TYPE: " + fbdo.dataType());
          }
          else {
            Serial.println("FAILED");
            Serial.println("REASON: " + fbdo.errorReason());
          }
          
          // // if is_uploading == 0, then set is_uploading = 1
          // if (is_uploading == 0){
          //   if (Firebase.RTDB.setInt(&fbdo, "/heartbeat_data/is_uploading", 1)){
          //     Serial.println("PASSED");
          //     Serial.println("PATH: " + fbdo.dataPath());
          //     Serial.println("TYPE: " + fbdo.dataType());
          //     Serial.println("isUploading set to 1");
          //   }
          //   else {
          //     Serial.println("FAILED");
          //     Serial.println("REASON: " + fbdo.errorReason());
          //   }
          // }
          
          // clear the json array for reuse
          clear_json_array(&json_array);
          Serial.println(duration);
          batch++;
        }


      }
    
    }

    isRecording = 0;
    if (Firebase.RTDB.setInt(&fbdo, "/heartbeat_data/is_recording", 0)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println("isRecording set to 0");
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // mark stop uploading to REALTIMEBATABASE
    if (Firebase.RTDB.setInt(&fbdo, "/heartbeat_data/is_uploading", 1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println("isUploading set to 1");
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

/*
Issues:
What if bumitaw, we need to record only the values in a batch with > 90000.
Discard the 256 at the end.
We need to update the data structure such that it records time as well.

How do we process this data?
**/