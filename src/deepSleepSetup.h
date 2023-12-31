#include <Arduino.h>

#ifdef needDeepSleep
#define minutes   3

// Accion a ejecutar antes de entrar en deepSleep
void (*beforeDeepSleepCallback)(void);

// Timing y Deep Sleep
unsigned long lastTime = 0;
unsigned long diferencia = 0;
void deepSleepSetup() {
  lastTime = millis();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);
}
void doSleep(){
  beforeDeepSleepCallback();
  delay(1000);// Evitar que el boton lo vuelva a encender
  esp_deep_sleep_start();
}
void handleDeepSleepLoop() {
  // Deep Sleep Timing in 2 minutes
  unsigned long currentIme = millis();
  diferencia = currentIme - lastTime;
  if(diferencia >= 1000 * 60 * minutes){
    doSleep();
  }
}

unsigned long lastTimeOneSecond = 0;
bool handleOneSecond() {
  unsigned long currenTime = millis();
  if(currenTime - lastTimeOneSecond >= 1000){
    lastTimeOneSecond = currenTime;
    return true;
  }
  return false;
}

void saveTime() {
  lastTime = millis();
}

boolean longPress(int value) {
  unsigned long currentIme = millis();
  diferencia = currentIme - lastTime;
  return diferencia >= value;
}

#else

void (*beforeDeepSleepCallback)(void);
void deepSleepSetup() {}
void beforeDeepSleep() {}
void handleDeepSleepLoop() {}
void saveTime() {}
boolean longPress() {return false;}
void doSleep(){}

#endif