// Libraries
#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
// #include <stdio.h>
// Wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
// Firebase
#include <Firebase_ESP_Client.h>
// #include <FirebaseJson.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// Time
#include "time.h"

// Network credentials
#define WIFI_SSID "GlobeAtHome_8A984_2.4"
#define WIFI_PASSWORD "MCm7fGGY"

// Firebase credentials
#define API_KEY "AIzaSyC21Lyo6PDNBpShPR1b8PZ2HreeaTwRpa0"
#define DATABASE_URL "https://test1-a4e94-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Firebase variables
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Ali Jafri's Ring Buffer
#include <bits/stdc++.h>

#define BUFFER_MAX_LEN 20 // must be changed since malalaking values nga naman

typedef struct RingBuffer {
    int i; // index to insert item
    int arr[BUFFER_MAX_LEN];
} RingBuffer;

void init_ring_buffer(RingBuffer *buffer){
    buffer->i = 0;
    for (int i = 0; i < BUFFER_MAX_LEN; i++)
        buffer->arr[i] = 0;
}

int insert_ring_buffer(RingBuffer *buffer, int item){
   buffer->arr[buffer->i] = item; 
   buffer->i = (buffer->i + 1) % BUFFER_MAX_LEN;
   if (buffer->i == 0) return 1;
   return 0;
}

// for debugging
void print_ring_buffer(RingBuffer *buffer){
    for (int i = 0; i < BUFFER_MAX_LEN; i++)
      printf("%d ", buffer->arr[i]);
    printf("\n");
}

void clear_json_array(FirebaseJsonArray *json_array){
  for (int i = 0; i < json_array->size(); i++)
      json_array->remove(i);
}

// int main(){
//     RingBuffer buffer;
//     init_ring_buffer(&buffer);
//     for (int i = 0; i < 2 * BUFFER_MAX_LEN; i++){
//         insert_ring_buffer(&buffer, i + 1);
//         print_ring_buffer(&buffer);
//     }
//     return 0;
// }
// siguro if ang inupadte ay ang last element ng ring buffer, doon lang magsesend

//

// data structures
RingBuffer subset;
int PULSE_RECORD_TIME_DURATION = 30000;
FirebaseJsonArray json_array;
char db_path[50]; // for the path of the data in the database
// hardware
MAX30105 particleSensor;

// i want a function that 


void setup(){
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

  
  /**
  Setup Hardware components
  */

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

  //Arduino plotter auto-scales annoyingly. To get around this, pre-populate
  //the plotter with 500 of an average reading from the sensor

  //Take an average of IR readings at power up

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

  unsigned long startTime = millis();
  int batch = 0; // the batch number of the data

  while((millis() - startTime < PULSE_RECORD_TIME_DURATION)) {
    // Record the IR readings
    Serial.println("zandrew");
    int reading = particleSensor.getIR();

    if(reading > 90000 && insert_ring_buffer(&subset, reading)) { // if full yeah. how do u handle the case where the ring buffer is not full but time is up
      // send to firebase
      //// convert to proper array firebase in the package
      if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();    // send every second

        // transfer contents from RingBuffer to FirebaseJsonArray
        for (int j = 0; j <= subset.i; j++){
          json_array.add(subset.arr[j]);
        }
      
        // i want to change test/arr everytime like incrementing it

        // push the array to the database in a PATH
        sprintf(db_path, "test/arr%d", batch);
        if (Firebase.RTDB.pushArray(&fbdo, db_path, &json_array)) {
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }

        ////////////////////but i do not want to reallocate a new json_array to save space. i want to reuse the same json_array
        // clear the json array for reuse
        clear_json_array(&json_array);
      }


    }
    batch++;
  }
}
