// Tested on Arduino 1.6.9 (CC)

#include <Adafruit_NeoPixel.h>

// for changing between test modes
#define TEST_MODE 2

// Arduino pin number
#define NEO_PIN 14
// Number of LEDs in the NeoPixel strip, ring, etc
#define NEO_NUM 7

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//  NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//  NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//  NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//  NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//  NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
//  NEO_GRBW    Pixels are wired for GRBW bitstream
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEO_NUM, NEO_PIN, NEO_GRBW);

//
float outValue = 0.0;

void setup() {
  // Initialize serial communication with computer, for debugging
  Serial.begin(57600);

  // Initialize the strip object
  strip.begin();
  // Initialize all pixels to 'off'
  strip.show();

}

void loop() {
  // Read the input on analog pin 0:
  int sensorValue = analogRead(A0);

  if (TEST_MODE == 1) {
    // For slow transitions
    int desiredOutValue = 0;
    float desiredSpeed = 0;

    // if someone is detected
    if (sensorValue > 500) {
      // Set the output value to the maximum
      desiredOutValue = 255;
      desiredSpeed = 8.0;
    } else {
      // set the output value to the minimum
      desiredOutValue = 0;
      desiredSpeed = 64.0;
    }

    // Smoothly transition towards the desired value at the desired speed,
    // the higher the speed value the slower the transition
    float dif = desiredOutValue - outValue;
    if (abs(dif) > 1.0) {
      outValue = outValue + dif / desiredSpeed;
    }

    // Change the pixels out values
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, outValue, 0, 0, 50);
    }
    strip.show();

  } else {
    // For slow transitions
    int desiredOutValue = 0;
    float desiredSpeed = 0;

    // if someone is detected
    if (sensorValue > 500) {
      // Set the output value to the maximum
      desiredOutValue = 255;
      desiredSpeed = 8.0;
    } else {
      // Set the output value to the minimum
      desiredOutValue = 50;
      desiredSpeed = 64.0;
    }

    // Smoothly transition towards the desired value at the desired speed,
    // the higher the speed value the slower the transition
    float dif = desiredOutValue - outValue;
    if (abs(dif) > 1.0) {
      outValue = outValue + dif / desiredSpeed;
    }

    // Change the pixels out values
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0, outValue);
    }
    strip.show();

  }

  // Print out the value you read
  Serial.println(sensorValue, DEC);
  // Delay in between reads for stability
  delay(30);

}
