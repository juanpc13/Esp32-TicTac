int accionIndex = 1;

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
