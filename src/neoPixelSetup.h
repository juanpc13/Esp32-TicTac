#ifdef needNeoPixel
#include <Adafruit_NeoPixel.h>

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
#else

void neoBeforeBoot(){}
void neoAfterBoot(){}

#endif

