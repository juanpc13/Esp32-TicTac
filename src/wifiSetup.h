#ifdef needWifi
#include <WiFi.h>
#include <WiFiMulti.h>

boolean isWifiConected = false;
WiFiMulti wifiMulti;

void wifiSetup(){
    // Definiciones para Nombre del WIFI
    WiFi.setHostname("ESP32-TicTac");

    // Lista de WIFIS
    wifiMulti.addAP("CLARO1_8D831B", "606s2wxuxM");
    wifiMulti.addAP("Master", "87654321");
    //wifiMulti.addAP("NAME", "PASS");
    
    // ESPERAR QUE SE CONECTE
    if(wifiMulti.run() == WL_CONNECTED) {
        //Serial.println("");
        //Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());  
    }
    
}

boolean wifiLoop(){
    isWifiConected = wifiMulti.run() == WL_CONNECTED;
    return isWifiConected;
}

#else

boolean isWifiConected = false;
void wifiSetup(){}
boolean wifiLoop(){return isWifiConected;}

#endif

