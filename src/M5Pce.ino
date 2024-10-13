//    PC Engine emulator M5PCE for M5Stack by @shikarunochi 2021.12-
#ifdef ARDUINO_XIAO_ESP32S3
#include<Arduino.h>
#else
#include <M5Stack.h>
#include <M5StackUpdater.h>  // https://github.com/tobozo/M5Stack-SD-Updater/
#endif
#include "M5pce.h"
#include "pce.h"

void setup() {
#ifdef ARDUINO_XIAO_ESP32S3
  if (!SPIFFS.begin()) { 
    Serial.println("SPIFFS Mount Failed");
    return;
  }
#else
  M5.begin(true,true,true,true);  
  if(digitalRead(BUTTON_A_PIN) == 0) {
     Serial.println("Will Load menu binary");
     updateFromFS(SD);
     ESP.restart();
  }
  pinMode(GAMEBOY_INT_PIN, INPUT_PULLUP);
#endif
//  emuMain();
  String strFileName = pceSelect();
  if(strFileName.length() > 0){
    char fileName[255];
    strFileName.toCharArray(fileName, 255);
    main(fileName);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}