#ifdef _ESPAsyncWebServer_H_>
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end    
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)    
  } else if(type == WS_EVT_DATA){
    //data packet
    //client->text(txt);
    //client->text(data, len);
    String txt = (const char *) data;
  }
}


void webServerSetup(){
  //Iniciando la memoria SPIFFS
  SPIFFS.begin();
  //Servidor de Archivos de la memoria SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  //Agregando los eventos del WebSocket al Servidor
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  //Iniciar Servidor
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}
#endif
