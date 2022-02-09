#include <Arduino.h>

#define needWifi
#include <wifiSetup.h>

#define needOTA
#include <otaSetup.h>

#define needNeoPixel
#include <neoPixelSetup.h>

#define needDeepSleep
#include <deepSleepSetup.h>

//#define needWebServer
#include <webServerSetup.h>

#define needWebSocket
#include <webSocketSetup.h>

#include <accionesBtn.h>

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
  //delay(500);
  // Colores de la accion
  colorAccion();
}

// LOOPS SIN DELAY SI ESTAN ACTIVADOS
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
    saveTime();
    // Retardando el boton
    while(digitalRead(btn)){delay(10);}
    // Long Press more than 3 s
    if(longPress(3000)){
      // Apagar
      doSleep();
    // Long Press more than 600 ms
    }else if(longPress(600)){
      // Accion de larga duracion
      nextAccion();
    }else{
      // Accion normal
      btnAccion();
    }
    // Mostrar color de la accion
    colorAccion();
    delay(100);
  }
  lastPress = currentPress;
  
}