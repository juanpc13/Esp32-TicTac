#ifdef Arduino_h

// Timing y Deep Sleep
unsigned long lastTime = 0;
unsigned long diferencia = 0;
void deepSleepSetup() {
  lastTime = millis();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);
}
void handleDeepSleepLoop() {
  // Deep Sleep Timing in 2 minutes
  unsigned long currentIme = millis();
  diferencia = currentIme - lastTime;
  if(diferencia >= 1000 * 60 * 2){
    esp_deep_sleep_start();
  }
}
void activo() {
  lastTime = millis();
}

#endif