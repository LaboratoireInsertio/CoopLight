// Tested on Arduino 1.6.9 (CC)
// For instruction on how to get the Feather HUZZAH running check:
// https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

#include <Adafruit_NeoPixel.h>

// for changing between output test modes
// 1 : Red lights with presence
// 2 : White intensity changes with presence
#define OUTPUT_MODE 1
// for changing between sensore modes:
// 1 : IR Distance Sensor
// 2 : PIR Motion Sensor
#define SENSOR_MODE 2

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
float outValueRed = 0.0;
float outValueWhite = 0.0;

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
    int desiredOutValueRed = 0;
    float desiredSpeedRed = 0;
    int desiredOutValueWhite = 0;
    float desiredSpeedWhite = 0;

    // if someone is detected
    if (presence) {
      // Set the output value to the maximum
      desiredOutValueRed = 255;
      desiredSpeedRed = 8.0;
      desiredOutValueWhite = 100;
      desiredSpeedWhite = 64.0;
    } else {
      // set the output value to the minimum
      desiredOutValueRed = 0;
      desiredSpeedRed = 64.0;
      desiredOutValueWhite = 255;
      desiredSpeedWhite = 64.0;
    }

    // Smoothly transition towards the desired value at the desired speed,
    // the higher the speed value the slower the transition
    float difRed = desiredOutValueRed - outValueRed;
    if (abs(difRed) > 1.0)
      outValueRed = outValueRed + difRed / desiredSpeedRed;
    float difWhite = desiredOutValueWhite - outValueWhite;
    if (abs(difWhite) > 1.0)
      outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

    // Change the pixels out values
    for (uint16_t i = 0; i < strip.numPixels(); i++)
      strip.setPixelColor(i, outValueRed, 0, 0, outValueWhite);
    strip.show();

  } else {
    // For slow transitions
    int desiredOutValueWhite = 0;
    float desiredSpeedWhite = 0;

    // if someone is detected
    if (presence) {
      // Set the output value to the maximum
      desiredOutValueWhite = 255;
      desiredSpeedWhite = 8.0;
    } else {
      // Set the output value to the minimum
      desiredOutValueWhite = 50;
      desiredSpeedWhite = 64.0;
    }

    // Smoothly transition towards the desired value at the desired speed,
    // the higher the speed value the slower the transition
    float difWhite = desiredOutValueWhite - outValueWhite;
    if (abs(difWhite) > 1.0)
      outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

    // Change the pixels out values
    for (uint16_t i = 0; i < strip.numPixels(); i++)
      strip.setPixelColor(i, 0, 0, 0, outValueWhite);
    strip.show();

  }

  // Print out the value you read
  //Serial.println(presence, DEC);
  // Delay in between reads for stability
  delay(30);
}

