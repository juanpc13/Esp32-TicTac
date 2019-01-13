#include "TicTac.h"

void setup() {

  //Inicializamos los pines
  pinMode(externalPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);

  //Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "...Cargando...");
  display.display();

  //Iniciando mi Sistema de archivos
  uFileS.begin(FORMAT_SPIFFS);

  //Configurando myWiFi
  myWiFi.setWifiFile(wifiFile);
  myWiFi.setFileSystem(&uFileS);
  myWiFi.begin();
  wifiMulti.run();

  display.clear(); display.display();
}

void loop() {

  boolean conected = wifiMulti.run();
  byte switchValue = digitalRead(switchPin);

  display.clear();

  if (switchValue == LOW && conected) {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, WiFi.localIP().toString());
    delay(10);
  }

  display.display();

}

