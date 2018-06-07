
#include <Adafruit_NeoPixel.h>


/*
 * Hier wird der LED Ring definiert
 */
#define NUMPIXELS   12
#define LEDPIN      2
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  
  pixels.begin(); // This initializes the NeoPixel library.

  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,150,0)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(150); // Delay for a period of time (in milliseconds).

  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
