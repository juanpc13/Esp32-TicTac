#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <neoPixelSetup.cpp>

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
  neoBeforeBoot();
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
  // Indicador para LOOP
  neoAfterBoot();
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
    delay(100);
  }
  lastPress = currentPress;
  
}