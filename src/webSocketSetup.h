//#define needWebSocket
#ifdef needWebSocket

#include <WebSocketsClient.h>
WebSocketsClient webSocket;

#define port 80
#define path "/ws"
#define hostname "192.168.1.2"

void webSocketSetup(){
    // Config WS Client
    // server address, port and URL
    webSocket.begin(hostname, port, path);
	webSocket.setReconnectInterval(5000);
}
void webSocketLoop(){
    webSocket.loop();
}
void webSocketSendAll(const char * payload){
    webSocket.sendTXT(payload);
}

#else

void webSocketSetup(){}
void webSocketLoop(){}
void webSocketSendAll(const char * payload){}

#endif