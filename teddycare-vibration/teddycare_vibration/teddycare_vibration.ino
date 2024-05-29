#include <Arduino.h>
// #include <WiFi.h>
// #include <WiFiClient.h>
// #include <WiFi.h>
// Firebase
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "SILVER"
#define WIFI_PASSWORD "dy3fao123"

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define API_KEY "AIzaSyC21Lyo6PDNBpShPR1b8PZ2HreeaTwRpa0"
#define DATABASE_URL "https://test1-a4e94-default-rtdb.asia-southeast1.firebasedatabase.app/" 
// Firebase variables
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;


// Data structures
#define BUFFER_MAX_LEN 100
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

// const int motor_pin = D8;
// const int motor_two = D9;
const int motor_pin = 22;
const int motor_two = 23;
const int HIGHER = 255; // assume 20 is lowest PWM state in this hardware config
const int LOWER = 0;

int PULSE_RECORD_TIME_DURATION = 30000;
FirebaseJsonArray json_array;
FirebaseJson json_object;
FirebaseJsonData result; // object that keeps the deserializing result
char db_path[70]; // for the path of the data in the database
PointArray long_array;

int batch = 100;
int is_recording = 0;
int is_vibrating = 0;
int counter = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long rest_interval = 500;
unsigned long beat_interval = 20;
unsigned long war_interval = 25;




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

  //D.S
  init_pt_array(&long_array);
    pinMode(motor_pin,OUTPUT);  // set motor pin as output
  pinMode(motor_two, OUTPUT); //set bigger group output
}

void loop(){     
  counter = 0; //reset the counter value for a clean life.
    if (Firebase.RTDB.getInt(&fbdo, "/heartbeat_data/is_vibrating")) {
      if (fbdo.dataType() == "int") {
        is_vibrating = fbdo.intData();
        Serial.print("VALUE OF ISVIBRATING IS");
        Serial.print(is_vibrating);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }


  if (is_vibrating) {
    while(1){
      sprintf(db_path, "/heartbeat_data/preprocessed_heartbeat/spikes/%d", counter);
      Serial.println(counter);
      if (Firebase.RTDB.getJSON(&fbdo, db_path)) {
        const String rawData = fbdo.payload();
        delay(20);
          if (json_object.setJsonData(rawData)) {
            // Deserialize the json object and insert to the array
            DataPoint new_dp;          
            json_object.get(result, "delay"); // delay
            if (result.success) new_dp.delay = result.to<int>();
          
            json_object.get(result, "ir_value"); //ir value'
            if (result.success) new_dp.ir_value = result.to<int>();
            
            json_object.get(result, "peak"); //peak
            if (result.success) new_dp.peak = result.to<int>();
            
            new_dp.valid = 1;
            // Serial.println(new_dp.delay);
            // Serial.println(new_dp.ir_value);
            // Serial.println(new_dp.peak);
            insert_pt_array(&long_array, new_dp);

          }
      } else {
        Serial.println("Error in fetching the JSON Object! ");
        break;
      } 
      counter++;
    }

    // Mechanical actuation

    // baka ifetch ulit yung is_vibrating everytime, 
    int index = 0;
    while(1) {
      if (long_array.arr[index].valid == 1) {
        currentMillis = millis();
        previousMillis = currentMillis;   
        if(long_array.arr[index].peak == 1) { //peak
          while(currentMillis - previousMillis < war_interval) {
            currentMillis = millis();
            analogWrite(motor_pin, HIGHER);
            analogWrite(motor_two, HIGHER);
            Serial.println("ON");
            // previousMillis = currentMillis;
          }
        } else { // trough
          while(currentMillis - previousMillis < beat_interval) {
            currentMillis = millis();
            analogWrite(motor_pin, HIGHER);
            // previousMillis = currentMillis;
            Serial.println("pwede na level");
          }
        }

        currentMillis = millis();
        previousMillis = currentMillis;
        // Rest Area  
        while(currentMillis - previousMillis < rest_interval) {
          currentMillis = millis();
          analogWrite(motor_two, LOW);
          analogWrite(motor_pin, LOW);
          Serial.println("iminhere");
          // previousMillis = currentMillis;
        }
        index++;
      }
      else {
        break;
      }
    }
  }
    
}

    // Dapat may array tayo eh.
