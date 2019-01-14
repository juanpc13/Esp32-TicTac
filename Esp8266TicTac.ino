#include "TicTac.h"

void setup() {

  //Inicializamos los pines
  pinMode(externalPin, OUTPUT);
  switchButton.begin();

  //Configuracion de mi interfaz UI
  ui.setTargetFPS(60);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();
  ui.init();
  display.flipScreenVertically();

  //Iniciando mi Sistema de archivos
  uFileS.begin(FORMAT_SPIFFS);

  //Configurando myWiFi
  myWiFi.setWifiFile(wifiFile);
  myWiFi.setFileSystem(&uFileS);
  myWiFi.begin();

  wifiMulti.run();

}

void loop() {

  boolean conected = wifiMulti.run();
  

  int remainingTimeBudget = ui.update();
}
