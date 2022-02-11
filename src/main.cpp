#include <Arduino.h>

#define needWifi
#include <wifiSetup.h>

#define needOTA
#include <otaSetup.h>

#define needDeepSleep
#include <deepSleepSetup.h>

#define needMqtt
#include <mqttSetup.h>

int brillo = 0;

int lastPress = 0;
#include <accionesBtn.h>

void setup() {
  //Serial.begin(115200);
  neoBeforeBoot();
  wifiSetup();
  otaSetup();
  deepSleepSetup();
  // MQTT
  mqttSetup();
  // Indicador para LOOP
  neoAfterBoot();  
  btnSetup();
}

// LOOPS SIN DELAY SI ESTAN ACTIVADOS
void servicios(){
  // Loops para OTA
  otaLoop();
  // Loop MQTT
  mqttLoop();
}

void loop() {
  ////// LOOPS CON INTERNET
  if(wifiLoop()){

    //Loop de servicios sin DELAY
    servicios();
    
  }

  // Manejando Botones
  btnLoop();
  
  // Loop Deep Sleep
  handleDeepSleepLoop();
}