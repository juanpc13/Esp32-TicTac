#include <Arduino.h>

#ifdef ADAFRUIT_NEOPIXEL_H>

#define PIN         14
#define NUMPIXELS   1
Adafruit_NeoPixel   pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

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
#endif

