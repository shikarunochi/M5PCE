//    PC Engine emulator M5PCE for M5Stack by @shikarunochi 2021.12-
#ifndef M5PCE_H
#define M5PCE_H
#include "common.h"
#ifdef ARDUINO_XIAO_ESP32S3
#include<Arduino.h>
#include "FS.h"
#include <SPIFFS.h>
#else
#include <M5Stack.h>
#endif
#define TRACE Serial.printf
#define MAX_FILES 255

#define GAMEBOY_KEY_NONE        0x00
#define GAMEBOY_KEY_RELEASED    0xFF
#define GAMEBOY_KEY_START       0x7F
#define GAMEBOY_KEY_SELECT      0xBF
#define GAMEBOY_KEY_A           0xEF
#define GAMEBOY_KEY_B           0xDF
#define GAMEBOY_KEY_UP          0xFE
#define GAMEBOY_KEY_DOWN        0xFD
#define GAMEBOY_KEY_LEFT        0xFB
#define GAMEBOY_KEY_RIGHT       0xF7

#define GAMEBOY_I2C_ADDRESS 0x08
#define GAMEBOY_INT_PIN 5

String pceSelect();
#endif