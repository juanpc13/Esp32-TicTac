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
  delay(1000);
  display.clear();display.display();
   
}

void loop() {

  switchButton.update();
  boolean conected = wifiMulti.run();
  byte switchValue = digitalRead(switchPin);

  if (switchButton.doubleSwitch() == 1) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    count++;
    display.drawString(64, 10, String(count));
    display.drawString(64, 22, WiFi.localIP().toString());
    display.display();
  }

  if (switchButton.doubleSwitch() == 2) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, String(millis() / 1000) + "s");
    display.display();
  }

}

