void batteryOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  int batteryPercentage = map(ESP.getVcc(), 2900, 3500, 0, 100);
  batteryPercentage = constrain(batteryPercentage, 0, 100);
  display->drawString(128, 0, String(batteryPercentage) + "%");
}

void infoDeviceFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  
  switchButton.update();  
  
  switch (switchButton.doubleSwitch()) {
    case 1:
      ui.nextFrame();
      break;

    case 2:

      break;
  }

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0 + y, "Informacion");
  display->drawString(64 + x, 11 + y, String(millis()));
  display->drawString(64 + x, 21 + y, (WiFi.SSID() == "") ? "-" : WiFi.SSID());
  display->drawString(64 + x, 31 + y, WiFi.localIP().toString());
}

void wifiListFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  switchButton.update();
  
  switch (switchButton.doubleSwitch()) {
    case 1:
      ui.nextFrame();
      break;

    case 2:
      n = WiFi.scanNetworks();
      break;
  }

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0 + y, "Buscar Wifi");
  if (n == 0) {
    display->drawString(64 + x, 10 + y, "-Vacio-");  
  } else {
    String s = "";
    for (int i = 0; i < n; ++i) {
      s += String(i + 1);
      s += ".";
      s += WiFi.SSID(i);
      /*s += " (";
      s += WiFi.RSSI(i);
      s += ")";
      */
      s += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
      
      s += "\n";
    }
    display->drawString(64 + x, 10 + y, s);
  }

}
