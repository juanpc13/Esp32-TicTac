#include <Wire.h>
#include "SH1106.h"
#include "SH1106Wire.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//Mi Gestor de archivos
#include "utilsSPIFFS.h"
utilsSPIFFS uFileS;

//Archivo con la lista de Wifis
const String wifiFile = "/wifi_list.json";

//Iniciando wifiMulti
ESP8266WiFiMulti wifiMulti;

//Mi gestor de Wifi
#include "MyWiFi.h"
MyWiFi myWiFi;

//Pantalla
SH1106 display(0x3c, 0, 2);

//Extras
#define FORMAT_SPIFFS false
const byte switchPin = 1;
const byte externalPin = 3;

