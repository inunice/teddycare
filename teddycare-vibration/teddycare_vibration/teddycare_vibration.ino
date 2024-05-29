#include <Arduino.h>
#include <WiFi.h>
// Firebase
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define API_KEY "AIzaSyCYMkG_fXoxCRsKImpuWSHvSOZq_zv1fJU"
#define DATABASE_URL "https://teddycare-12aaf-default-rtdb.asia-southeast1.firebasedatabase.app/"
// Firebase variables
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;


// Data structures
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

const int MOTOR_TWO_PIN = 23;
const int PWM_CHANNEL_TWO = 1; 
const int PWM_FREQ = 5000;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8; // We'll use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits 

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

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
  // MOTOR_PIN_tWO
  ledcSetup(PWM_CHANNEL_TWO, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_TWO_PIN, PWM_CHANNEL_TWO);
}

void loop(){     
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
        delay(50);
      } else {Serial.println("Error in fetching the JSON Object! ");break;}
    }
    // Mechanical actuation
    int index = 0;
    while(is_vibrating) {
      if (long_array.arr[index].valid == 1) {
        ledcWrite(PWM_CHANNEL_TWO, long_array.arr[index].ir_value);          
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
    
}