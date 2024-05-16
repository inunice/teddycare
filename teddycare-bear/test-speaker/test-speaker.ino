#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#define FPSerial Serial1

DFRobotDFPlayerMini myDFPlayer;

void setup()
{
  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/27, /*tx =*/26);   // Start sound player

  Serial.begin(115200);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
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

  myDFPlayer.volume(30);    // Volume value from 0 to 30
  myDFPlayer.play(1);       // Play the first file
}

void loop()
{
  static unsigned long timer = millis();
  if (millis() - timer > 30000) {   // Start after 30 seconds
    timer = millis();
    myDFPlayer.play(2);  // Play second file
  }
}