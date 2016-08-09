// Tested on Arduino 1.6.9 (CC)
// Adafruit HUZZAN not working on macOS Sierra

#include <Adafruit_NeoPixel.h>

// for changing between output test modes
#define OUTPUT_MODE 2
// for changind between sensore modes:
// 1 : IR Distance Sensor
// 2 : PIR Motion Sensor
#define SENSOR_MODE 1

// NeoPixel pin number
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

  // Declare pin 12 (PIR sensor) as an input
  pinMode(12, INPUT);

  // Initialize the strip object
  strip.begin();
  // Initialize all pixels to 'off'
  strip.show();

}

void loop() {
  boolean presence = false;
  if (SENSOR_MODE == 1) {
    // Read the input on analog pin 0:
    int sensorValue = analogRead(A0);

    if (sensorValue > 500)
      presence = true;
    else
      presence = false;

  } else {
    // Read the input on digital pin 12:
    int sensorValue = digitalRead(12);

    if (sensorValue == HIGH)
      presence = true;
    else
      presence = false;
  }

  if (OUTPUT_MODE == 1) {
    // For slow transitions
    int desiredOutValue = 0;
    float desiredSpeed = 0;

    // if someone is detected
    if (presence) {
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
    if (presence) {
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
  Serial.println(presence, DEC);
  // Delay in between reads for stability
  delay(30);

}
