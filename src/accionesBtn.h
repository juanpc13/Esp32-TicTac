int accionIndex = 1;

void nextAccion(){
  // Sigiente accion
  accionIndex += 1;
  if(accionIndex > 2){
    accionIndex = 1;
  }
}
void colorAccion(){
  switch (accionIndex){
  case 1:
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(10, 10, 0));
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
    webSocketSendAll("-1");
    break;
  case 2:
    // NO PROGRAMADO
    break;
  }
}
