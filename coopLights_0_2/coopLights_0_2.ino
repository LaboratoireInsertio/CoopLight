// CoopLights 0.2
//
// Uses the Feather M0 WIFI instead of the Feather HUZZAH due to
// problems of compatibility with capacitance sensing.
// Implements capacitance sensing.
// Abandons IR Distance Sensor implementation (Sticks with PIR).
//
// Tested on Arduino 1.6.10 (CC)
// For instruction on how to get the Feather M0 WIFI running check:
// https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/

#include <Adafruit_NeoPixel.h>
#include <RBD_Capacitance.h>


// for changing between output test modes
// 1 : Red lights with presence
// 2 : White intensity changes with presence
#define OUTPUT_MODE 1


// NeoPixel pin number
#define NEO_PIN 6
// Number of LEDs in the NeoPixel strip, ring, etc
#define NEO_NUM 7

// PIR sensor pin number
#define PIR_PIN 11

// Capacitance send and recieve pin numbers
#define CAPS_PIN 12
#define CAPR_PIN 5


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

// Parameter 1 = Send pin
// Parameter 2 = Recieve pin
RBD::Capacitance cap_sensor(CAPS_PIN, CAPR_PIN);

// Timer variable
unsigned long previousTime;
// Light transition frame rate in milliseconds
int interval = 30;
// Output values
float outValueRed = 0.0;
float outValueWhite = 0.0;

void setup() {
  // Initialize serial communication with computer, for debugging
  Serial.begin(115200);

  // Declare the PIR sensor pin as an input
  pinMode(PIR_PIN, INPUT);

  // Initialize the strip object
  strip.begin();
  // Initialize all pixels to 'off'
  strip.show();

}

void loop() {
  cap_sensor.update();

  boolean presence = false;

  // Read the input on PIR sensor pin:
  int sensorValue = digitalRead(PIR_PIN);

  if (sensorValue == HIGH)
    presence = true;
  else
    presence = false;


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

    if ((millis() - previousTime) > interval) {
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

      // Mark the time
      previousTime = millis();
    }

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

    if ((millis() - previousTime) > interval) {
      // Smoothly transition towards the desired value at the desired speed,
      // the higher the speed value the slower the transition
      float difWhite = desiredOutValueWhite - outValueWhite;
      if (abs(difWhite) > 1.0)
        outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

      // Change the pixels out values
      for (uint16_t i = 0; i < strip.numPixels(); i++)
        strip.setPixelColor(i, 0, 0, 0, outValueWhite);
      strip.show();

      // Mark the time
      previousTime = millis();
    }
  }

  // Print out the value you read
  //Serial.println(presence, DEC);

  // Print out the value of the capacitance sensor
  if (cap_sensor.onChange())
    Serial.println(cap_sensor.getValue());

  // Adding delays will prevent the capacitance sensor library from 
  // working properly! Use timers instead: 
  // http://wiring.org.co/learning/basics/timer.html
}

