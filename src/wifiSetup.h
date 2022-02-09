#ifdef needWifi
#include <WiFi.h>

IPAddress staticIP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

void wifiSetup(){
    const char* ssid = "CLARO1_8D831B";
    const char* password = "606s2wxuxM";
    
    WiFi.setHostname("ESP32-TicTac");
    WiFi.mode(WIFI_STA);
    /*
    if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
        Serial.println("Configuration failed.");
    }
    */   

    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  
}

#else

void wifiSetup(){}

#endif

