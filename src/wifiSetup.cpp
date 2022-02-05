#ifdef WiFi_h>
void wifiSetup(){
    const char* ssid = "CLARO1_8D831B";
    const char* password = "606s2wxuxM";
    
    WiFi.mode(WIFI_STA);
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
#endif

