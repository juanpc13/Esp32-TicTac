#include <Arduino.h>

#define needWifi
#include <wifiSetup.h>

#define needOTA
#include <otaSetup.h>

#define needDeepSleep
#include <deepSleepSetup.h>

//#define needMqtt
#include <mqttSetup.h>

int brillo = 0;

int lastPress = 0;
#include <data.h>
#include <accionesBtn.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
const uint16_t kIrLed = 25;
const uint16_t kIrkHz = 38;
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
    int randomNumber = random(1, 4);
    switch (randomNumber)
    {
      case 1:
        irsend.sendRaw(yellow, yellow_size, kIrkHz);
        break;
      case 2:
        irsend.sendRaw(blue, blue_size, kIrkHz);
        break;
      case 3:
        irsend.sendRaw(green, green_size, kIrkHz);
        break;
      case 4:
        irsend.sendRaw(pink, pink_size, kIrkHz);
        break;    
      default:
        break;
    }
  }

  // Manejando Botones
  btnLoop();
  
  // Loop Deep Sleep
  handleDeepSleepLoop();
}