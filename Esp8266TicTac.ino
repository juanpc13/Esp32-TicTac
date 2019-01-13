#include "TicTac.h"

void setup() {

  //Inicializamos los pines
  pinMode(externalPin, OUTPUT);
  switchButton.begin();

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

  switchButton.update();
  boolean conected = wifiMulti.run();
  byte switchValue = digitalRead(switchPin);

  display.clear();

  if (switchButton.isPressed() && conected) {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, WiFi.localIP().toString());
    if(switchButton.longPress()){
      display.drawString(64, 5, "LongPress Active");            
    }
  }

  display.display();

}

