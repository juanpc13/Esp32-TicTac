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
// Update from MQTT
boolean update = false;

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
  // Loop Deep Sleep
  handleDeepSleepLoop();
  // Loop MQTT
  mqttLoop();
}

void loop() {
  //Loop de servicios sin DELAY
  servicios();
  // My Code
  btnLoop();
  // Update from MQTT
  if(update){
    colorAccion();
    update = false;
  }
}