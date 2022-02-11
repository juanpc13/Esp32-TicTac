#include <Adafruit_NeoPixel.h>
// Declaraciones del NEOPIXE
#define PIN         14
#define NUMPIXELS   1
Adafruit_NeoPixel   pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// Declaraciones del boton
#define btn   4
int accionIndex = 1;

// POINTER DE FUNCION PARA ACTUALIZAR CUANDO RECIBA MENSAJE
void (*onPressCallback)(void);
void (*onLongPressCallback)(void);
void (*onLargePressCallback)(void);

void neoBeforeBoot(){
    pixels.begin();
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(5, 0, 0));
    pixels.show();
}
void neoAfterBoot(){
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(5, 5, 5));
    pixels.show();
}
void neoBeforeDeepSleep(){
    pixels.clear();
    pixels.show();
}

void nextAccion(){
  // Sigiente accion
  accionIndex += 1;
  if(accionIndex > 2){
    accionIndex = 1;
  }
}

void colorAccion(){
  int b = (brillo * 90);

  switch (accionIndex){
  case 1:
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(10 + b, 10 + b, 0));
    pixels.show();
    break;
  case 2:
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(10, 0, 10));
    pixels.show();
    break;
  }
}

void btnAccion(){
  switch (accionIndex){
  case 1:
    mqttPublish(pubPathMqtt, "-1");
    break;
  case 2:
    //mqttPublish(pathMqtt, "-1");
    break;
  }
}

void receivedMQTT(int &v){
  brillo = v;
  update = true;
}

void btnSetup(){
  // SE CONFIGURA BOTON
  pinMode(btn, INPUT);
  
  // SE ASGINA LA FUNCION A EJECUTAR SI SE PRESIONA 1 VEZ
  onPressCallback = &btnAccion;//ACCIONAR FUNCION

  // SE ASGINA LA FUNCION A EJECUTAR SI SE PRESIONA MAS DE 600ms
  onLongPressCallback = &nextAccion;//SIGUIENTE ITERACION

  // SE ASGINA LA FUNCION A EJECUTAR SI SE PRESIONA MUCHO TIEMPO 3s
  onLargePressCallback = &doSleep;//DEEPSLEEP

  // Antes de entrar a DeepSleep Callback
  beforeDeepSleepCallback = &neoBeforeDeepSleep;

  // SE CONFIGURA FUNCION CUANDO SE RECIBE POR MQTT
  receivedMqttCallback = &receivedMQTT;

  // SE ACTUALIZAN LOS COLORES
  colorAccion();
}

void btnLoop(){
  int currentPress = digitalRead(btn);
  if(lastPress == 0 && currentPress == 1){
    // Save timing
    saveTime();
    // Retardando el boton
    int tinyTiming = 0;
    while(digitalRead(btn)){
      delay(10);
      tinyTiming += 10;
      // Si tinyTiming excede los 3 segundos apagar
      if(tinyTiming > 3000){
        onLargePressCallback();//doSleep();
      }
    }
    // Long Press more than 600 ms
    if(longPress(600)){
      // Accion de larga duracion
      onLongPressCallback();//nextAccion();
    }else{
      // Accion normal
      onPressCallback();//btnAccion();
    }
    // Mostrar color de la accion
    colorAccion();
    delay(100);
  }
  lastPress = currentPress;
}
