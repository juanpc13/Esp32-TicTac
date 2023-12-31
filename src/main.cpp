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

#include <IRremoteESP8266.h>
#include <IRsend.h>
const uint16_t kIrLed = 25;
IRsend irsend(kIrLed);

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

  irsend.begin();
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

  if(handleOneSecond())
  {
    String incomingString = "[21]221112141321111224241,";//COLOR
    int startIndex = incomingString.indexOf('[') + 1;
    int endIndex = incomingString.indexOf(']');
    int newLength = incomingString.substring(startIndex, endIndex).toInt();
    uint16_t newRawData[newLength] = {};

    startIndex = incomingString.indexOf(']') + 1;
    endIndex = incomingString.indexOf(',');
    String newVals = incomingString.substring(startIndex, endIndex);
    for (int i = 0; i < newVals.length(); i++ ) {
      int intVal = newVals.substring(i, i + 1).toInt() * 700;
      newRawData[i] = intVal;
    }
    irsend.sendRaw(newRawData, newLength, 38);  // Send a raw data capture at 38kHz.
  }

  // Manejando Botones
  btnLoop();
  
  // Loop Deep Sleep
  handleDeepSleepLoop();
}