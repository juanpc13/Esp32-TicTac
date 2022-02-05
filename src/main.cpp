#include <Arduino.h>

#include <WiFi.h>
#include <wifiSetup.cpp>

#include <ArduinoOTA.h>
#include <otaSetup.cpp>

void setup() {
  Serial.begin(115200);
  wifiSetup();
  otaSetup();
}

void loop() {
  ArduinoOTA.handle();
  // Coding
}