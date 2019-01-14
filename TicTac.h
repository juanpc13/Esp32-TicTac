#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//Iniciando wifiMulti
ESP8266WiFiMulti wifiMulti;

#include "utilsSPIFFS.h"
#include "MyWifi.h"
MyWiFi myWiFi;
utilsSPIFFS uFileS;
const String wifiFile = "/wifi_list.json";

#include <Wire.h>
#include "SH1106.h"
#include "SH1106Wire.h"
#include "OLEDDisplayUi.h"
SH1106 display(0x3c, 0, 2);
OLEDDisplayUi ui(&display);

//Variables de timmer millis
unsigned long lastTime = 0;
unsigned long lastInactiveScreen = 0;
boolean screenActive = true;

//Extras
#define FORMAT_SPIFFS false
const byte switchPin = 1;
const byte externalPin = 3;
int count = 0;
int n = 0;
ADC_MODE(ADC_VCC);

//Objeto del boton
#include "ButtonTool.h"
ButtonTool switchButton(switchPin);

int frameCount = 2;
int overlaysCount = 1;
#include "images.h"
#include "myFrames.h"
OverlayCallback overlays[] = {batteryOverlay};
FrameCallback frames[] = {infoDeviceFrame, wifiListFrame};
