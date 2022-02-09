#include <Arduino.h>
#include <neoPixelSetup.cpp>

#include <WiFi.h>
#include <wifiSetup.cpp>

#include <ArduinoOTA.h>
#include <otaSetup.cpp>

#include <ESPAsyncWebServer.h>
#include <webServerSetup.cpp>

#include <deepSleepSetup.cpp>

#define needWebSocket
#include <webSocketSetup.h>

const int btn = 4;
int lastPress = 0;

void setup() {
  //Serial.begin(115200);
  neoBeforeBoot();
  wifiSetup();
  otaSetup();
  deepSleepSetup();
  //webServerSetup();
  // Config Pins
  pinMode(btn, INPUT);
	webSocketSetup();
  // Indicador para LOOP
  neoAfterBoot();
}

void loop() {
  // Loops para OTA y Deep Sleep
  ArduinoOTA.handle();
  handleDeepSleepLoop();
  webSocketLoop();
  // My Code
  int currentPress = digitalRead(btn);
  if(lastPress == 0 && currentPress == 1){
    // Save timing
    activo();
    webSocketSendAll("-1");
    delay(100);
  }
  lastPress = currentPress;
  
}