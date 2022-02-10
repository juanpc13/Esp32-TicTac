#ifdef needMqtt

#include <WiFiClientSecure.h>
#include <MQTT.h>
WiFiClientSecure net;
MQTTClient client;
#define uidMqtt "esp-tictac"
#define userMqtt "juanpc13"
#define passMqtt "a"
#define hostNameMqtt "juansv.com"
#define pathMqtt "America/El_Salvador/Santa_Ana/juanpc13/Casa1/Lampara1/Triac/"
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  //int result = payload.toInt();
}

void connect() {
    net.setInsecure();
    while (!client.connect(uidMqtt, userMqtt, passMqtt)) {
        Serial.print(".");
        delay(500);
        otaLoop();
        delay(500);
        otaLoop();
    }
    client.subscribe(pathMqtt);
}

void mqttSetup(){
    client.begin(hostNameMqtt, 8883, net);
    client.onMessage(messageReceived);
    connect();    
}

void mqttPublish(const char topic[], const char payload[]){
    client.publish(topic, payload);
}

void mqttLoop(){
    client.loop();
    //delay(10);  // <- fixes some issues with WiFi stability

    if (!client.connected()) {
        connect();
    }
}


#else
#define pathMqtt "/none"

void mqttSetup(){}
void connect(){}
void mqttPublish(const char topic[], const char payload[]){}
void mqttLoop(){}

#endif