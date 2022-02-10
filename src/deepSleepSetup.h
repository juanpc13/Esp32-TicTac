#ifdef needDeepSleep
#define minutes   1

// Accion a ejecutar antes de entrar en deepSleep
void (*beforeDeepSleep)(void);

// Timing y Deep Sleep
unsigned long lastTime = 0;
unsigned long diferencia = 0;
void deepSleepSetup() {
  lastTime = millis();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);
}
void doSleep(){
  beforeDeepSleep();
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
void saveTime() {
  lastTime = millis();
}

boolean longPress(int value) {
  unsigned long currentIme = millis();
  diferencia = currentIme - lastTime;
  return diferencia >= value;
}

#else

void (*beforeDeepSleep)(void);
void deepSleepSetup() {}
void beforeDeepSleep() {}
void handleDeepSleepLoop() {}
void saveTime() {}
boolean longPress() {return false;}
void doSleep(){}

#endif