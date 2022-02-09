#include <Arduino.h>

#define needWifi
#include <wifiSetup.h>

//#define needOTA
#include <otaSetup.h>

#define needDeepSleep
#include <deepSleepSetup.h>

#define needNeoPixel
#include <neoPixelSetup.h>

//#define needWebServer
#include <webServerSetup.h>
//#define needWebSocket
#include <webSocketSetup.h>

const int btn = 4;
int lastPress = 0;

void setup() {
  //Serial.begin(115200);
  neoBeforeBoot();
  wifiSetup();
  otaSetup();
  deepSleepSetup();
  webServerSetup();
  // Config Pins
  pinMode(btn, INPUT);
	webSocketSetup();
  // Indicador para LOOP
  neoAfterBoot();
}

void servicios(){
  // Loops para OTA
  otaLoop();
  // Loop Deep Sleep
  handleDeepSleepLoop();
  // Loop WebSocket
  webSocketLoop();  
}

void loop() {
  //Loop de servicios sin DELAY
  servicios();
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