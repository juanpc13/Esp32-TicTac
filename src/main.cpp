#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#define PIN       14
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#include <WiFi.h>
#include <wifiSetup.cpp>

#include <ArduinoOTA.h>
#include <otaSetup.cpp>

#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <webServerSetup.cpp>

#include <deepSleepSetup.cpp>

#include <WebSocketsClient.h>
WebSocketsClient webSocket;

const int btn = 4;
int lastPress = 0;

void setup() {
  //Serial.begin(115200);
  pixels.begin();
  wifiSetup();
  otaSetup();
  deepSleepSetup();
  //webServerSetup();
  // Config Pins
  pinMode(btn, INPUT);
  // Config WS Client
  // server address, port and URL
	webSocket.begin("192.168.1.2", 80, "/ws");
	webSocket.setReconnectInterval(5000);
}

void loop() {
  // Loops para OTA y Deep Sleep
  ArduinoOTA.handle();
  handleDeepSleepLoop();
  webSocket.loop();
  // My Code
  int currentPress = digitalRead(btn);
  if(lastPress == 0 && currentPress == 1){
    // Save timing
    activo();
    webSocket.sendTXT("-1");
  }
  lastPress = currentPress;

  // NeoPixel
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(10, 10, 10));
  pixels.show();
  
}