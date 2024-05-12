#include "Arduino.h"

#define SOUND_COOLDOWN 1000                // Gap between crying readings (60000 = 1 minute)
bool signupOK = false;

// Time
const char* ntpServer = "time.nist.gov";
const long  gmtOffset_sec = 28800; // GMT+8
const int   daylightOffset_sec = 0;

// Variables
unsigned int isCrying = 0;
unsigned int cryingCoolDown = SOUND_COOLDOWN;
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
  // Store to database ONLY if crying is 1 and not on cooldown
  if ((isCrying == 1) && (cryingCoolDown == SOUND_COOLDOWN)) {
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)){
      sendDataPrevMillis = millis();    // send every second
      Serial.println("Ready to send to database.");
    
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

  // Delay between readings!
  delay(1000);
}
