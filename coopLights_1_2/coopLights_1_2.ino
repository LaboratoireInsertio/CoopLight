/* CoopLights 1.2 Stable
   Developed and tested on Arduino 1.6.10 (CC)

   1.0: Stable version. Runs on the Feather M0 WIFI. Uses a PhotoCell
   at the bottom of the lamp to detect if it has been lifted and PIR
   sensors for presence. Currently works with the 7 pixel NeoPixel
   Jewel.

   1.1: Implements SLEEP mode: The lamp turns off after a given
   period of inactivity and turns on again when movement is
   detected. BEAT mode threshold auto calibrates during SLEEP.

   1.2: Stable version. Battery life and auto calibration check. 

   • For instruction on how to get the Feather M0 WIFI running check:
   https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/
   WARNING: When using the WIFI functionalities be careful not to
   upload private usernames or passwords to the GitHub!
*/

/**************** Battery Voltage Monitoring Setup ****************/
// Battery voltage monitoring pin number
#define VBAT_PIN 9 // Should be A7 but it apears not to be declared

// Battery voltage update timer
unsigned long lastVoltageCheck = 0;
// Update interval in milliseconds
// 5 min = 300000 millis
int voltageCheckInterval = 10000;

/************************* NeoPixel Setup *************************/
// Include the NeoPixel library
#include <Adafruit_NeoPixel.h>
// NeoPixel pin number
#define NEO_PIN 13
// Number of LEDs in the NeoPixel strip, ring, etc
// NeoPixel Jewel:  7
// NeoPixel Ring:  16
// NeoPixel FeatherWing:  32
#define NEO_NUM 7
// Type of NeoPixel
//  NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//  NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//  NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//  NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//  NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
//  NEO_GRBW    Pixels are wired for GRBW bitstream (NeoPixel Jewel, Ring)
#define NEO_TYPE NEO_GRBW

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEO_NUM, NEO_PIN, NEO_TYPE);

// NeoPixel update timer
unsigned long previousLedUpdateTime;
// Light transition frame rate in milliseconds
int ledUpdateInterval = 30;
// Output values
float outValueRed = 0.0;
float outValueWhite = 0.0;

// Used only for battery tracking
float oG = 0.0;
float oB = 0.0;

/************************ PIR Sensor Setup ************************/
// PIR sensor pin number
#define PIR_PIN 12

/************************ Photocell Setup ************************/
// Photocell pin number
#define PHOTO_PIN 12
// to store the photocell readings
int photoRead = 0;
// photocell threshold to go into BEAT mode
int photoThreshold = 1024;
// to control the frequency of the readings (will cause trouble if
// is less than 200 ms)
int photoInterval = 300;
unsigned long lastPhotoReading;

/************************** Calibration **************************/
#include <Average.h>

#define SAMPLE_SIZE 20
Average<uint16_t> calSamples(SAMPLE_SIZE);

boolean isCalibrating = false;
unsigned long lastCalibration = 0;
// 1 min = 60000 millis
// 5 min = 300000 millis
// 15 min = 900000 millis
unsigned int calibrationInterval = 600000;

int samplesTaken = 0;

/************************* Sleep Control *************************/
// Last time activity was detected
unsigned long lastTimeActive = 0;
// Time in milliseconds after which the lamp should go to sleep
// 1 min = 60000 millis
// 10 min = 600000 millis
// 30 min = 1800000 millis
unsigned int sleepInterval = 1800000;

/***************************** Beat *****************************/
// Beat phases
#define DIASTOLE_ONE 1
#define DIASTOLE_TWO 2
#define SYSTOLE_ONE 3
#define SYSTOLE_TWO 4
// Current phase
uint8_t phase = DIASTOLE_ONE;

// Current heart rate in beats per minute (BPM)
int bpm = 60;
// Beat frequency en milliseconds
int beatInterval = 60000 / bpm;
// Time of the last beat
long lastBeatTime = 0;
// Duration of each phase
int phaseInterval = 100;
// Time of the last phase
long lastPhaseTime = 0;

uint8_t bpms[] = {
  94, 94, 83, 83, 82, 82, 80, 80, 80, 78, 78, 79, 83, 84, 86, 87, 88, 88, 88, 88, 89,
  89, 87, 86, 86, 86, 85, 85, 85, 83, 82, 79, 86, 87, 82, 82, 82, 83, 84, 80, 73, 72,
  71, 70, 73, 74, 75, 75, 79, 80, 81, 81, 81, 74, 72, 72, 74, 75, 75, 75, 76, 76, 76,
  78, 87, 92, 94, 104, 116, 122, 120, 119, 121, 124, 120, 117, 116, 116, 116, 122,
  123, 124, 125, 127, 130, 132, 132, 121, 115, 113, 113, 115, 116, 119, 122, 123,
  123, 124, 121, 121, 121, 119, 115, 115, 111, 111, 111, 119, 119, 119, 119, 119,
  119, 118, 116, 116, 119, 123, 124, 124, 124, 122, 122, 120, 121, 114, 118, 120,
  117, 112, 111, 116, 116, 125, 122, 121, 122, 123, 125, 125, 125, 126, 126, 122,
  116, 119, 119, 119, 123, 128, 134, 133, 130, 124, 124, 124, 122, 123, 124, 122,
  123, 126, 127, 127, 127, 122, 120, 117, 116, 116, 116, 116, 116, 116, 116, 116,
  116, 148, 148, 147, 148, 148, 148, 147, 142, 139, 140, 143, 145, 146, 146, 145,
  148, 148, 144, 132, 131, 127, 126, 122, 122, 124, 123, 123, 120, 119, 118, 116,
  113, 113, 113, 106, 105, 104, 99, 96, 92, 88, 90, 99, 99, 99, 111, 109, 108, 108,
  108, 102, 99, 94, 94, 91, 92, 94, 95, 100, 98, 96, 95, 120, 115, 113, 111, 109,
  108, 108, 108, 108, 108, 105, 103, 105, 105, 102, 98, 97, 99, 97, 96, 95, 94, 93,
  93, 94, 94, 94, 92, 84, 84, 84, 83, 81, 82, 82, 82, 84, 84, 84, 95, 99, 100, 104,
  102, 100, 100, 100, 100, 100, 94, 89, 88, 86, 85, 85, 84, 85, 86, 87, 87, 90, 91,
  91, 91, 91, 90, 89, 88, 87, 83, 81, 84, 85, 87, 93, 93, 93, 93, 91, 94, 105, 105,
  100, 100, 101, 104, 103, 101, 100, 100, 100, 102, 103, 103, 103, 103, 105, 105,
  103, 102, 104, 112, 113, 113, 113, 113, 113, 115, 115, 114, 115, 115, 117, 119,
  118, 119, 120, 123, 123, 123, 123, 126, 126, 126, 120, 116, 113, 110, 110, 110,
  110, 111, 112, 112, 112, 112, 111, 111, 110, 110, 112, 115, 119, 120, 118, 118,
  118, 114, 118, 119, 120, 121, 121, 121, 121, 116, 113, 112, 111, 111, 108, 107,
  107, 102, 107, 108, 103, 100, 99, 98, 97, 95, 91, 91, 91, 88, 86, 94, 98, 103,
  103, 102, 105, 115, 116, 120, 122, 125, 127, 127, 121, 119, 116, 121, 118, 116,
  106, 106, 94, 93, 91, 87, 83, 82, 82, 82, 82, 81, 82, 82, 82, 82, 79, 81, 77, 77,
  75, 76, 80, 89, 89, 89, 99, 99, 99, 105, 106, 109, 111, 113, 114, 120, 115, 111,
  107, 97, 95, 92, 106, 128, 131, 132, 138, 139, 134, 133, 132, 131, 130, 124,
  124, 124, 122, 120, 116, 116, 113, 115, 119, 124, 124, 126, 127, 127, 127, 127,
  127, 127, 130, 131, 133, 135, 136, 134, 133, 131, 131, 131, 131, 131, 131, 130,
  130, 127, 126, 122, 122, 122, 123, 128, 130, 132, 131, 129, 126, 126, 126, 126,
  128, 128, 128, 128, 134, 134, 134, 134, 134, 133, 131, 131, 131, 131, 140, 140,
  140, 143, 141, 135, 130, 125, 128, 134, 134, 133, 133, 133, 133, 133, 133, 132,
  131, 131, 135, 138, 141, 141, 146, 142, 138, 138, 138, 148, 147, 147, 147, 144,
  144, 148, 147, 146, 143, 145, 140, 140, 145, 147, 145, 145, 144, 144, 145, 146,
  148, 141, 145, 144, 138, 138, 138, 122, 122, 122, 122, 121, 120, 130, 143, 143,
  144, 144, 144, 149, 150, 150, 150, 149, 152, 152, 150, 148, 148, 148, 148, 148,
  94, 94, 83, 83, 82, 82, 80, 80, 80, 78, 78, 79, 83, 84, 86, 87, 88, 88, 88, 88, 89,
  89, 87, 86, 86, 86, 85, 85, 85, 83, 82, 79, 86, 87, 82, 82, 82, 83, 84, 80, 73, 72,
  71, 70, 73, 74, 75, 75, 79, 80, 81, 81, 81, 74, 72, 72, 74, 75, 75, 75, 76, 76, 76,
  78, 87, 92, 94, 104, 116, 122, 120, 119, 121, 124, 120, 117, 116, 116, 116, 122,
  123, 124, 125, 127, 130, 132, 132, 121, 115, 113, 113, 115, 116, 119, 122, 123,
  123, 124, 121, 121, 121, 119, 115, 115, 111, 111, 111, 119, 119, 119, 119, 119,
  119, 118, 116, 116, 119, 123, 124, 124, 124, 122, 122, 120, 121, 114, 118, 120,
  117, 112, 111, 116, 116, 125, 122, 121, 122, 123, 125, 125, 125, 126, 126, 122,
  116, 119, 119, 119, 123, 128, 134, 133, 130, 124, 124, 124, 122, 123, 124, 122,
  123, 126, 127, 127, 127, 122, 120, 117, 116, 116, 116, 116, 116, 116, 116, 116,
  116, 148, 148, 147, 148, 148, 148, 147, 142, 139, 140, 143, 145, 146, 146, 145,
  148, 148, 144, 132, 131, 127, 126, 122, 122, 124, 123, 123, 120, 119, 118, 116,
  113, 113, 113, 106, 105, 104, 99, 96, 92, 88, 90, 99, 99, 99, 111, 109, 108, 108,
  108, 102, 99, 94, 94, 91, 92, 94, 95, 100, 98, 96, 95, 120, 115, 113, 111, 109,
  108, 108, 108, 108, 108, 105, 103, 105, 105, 102, 98, 97, 99, 97, 96, 95, 94, 93,
  93, 94, 94, 94, 92, 84, 84, 84, 83, 81, 82, 82, 82, 84, 84, 84, 95, 99, 100, 104,
  102, 100, 100, 100, 100, 100, 94, 89, 88, 86, 85, 85, 84, 85, 86, 87, 87, 90, 91,
  91, 91, 91, 90, 89, 88, 87, 83, 81, 84, 85, 87, 93, 93, 93, 93, 91, 94, 105, 105,
  100, 100, 101, 104, 103, 101, 100, 100, 100, 102, 103, 103, 103, 103, 105, 105,
  103, 102, 104, 112, 113, 113, 113, 113, 113, 115, 115, 114, 115, 115, 117, 119,
  118, 119, 120, 123, 123, 123, 123, 126, 126, 126, 120, 116, 113, 110, 110, 110,
  110, 111, 112, 112, 112, 112, 111, 111, 110, 110, 112, 115, 119, 120, 118, 118,
  118, 114, 118, 119, 120, 121, 121, 121, 121, 116, 113, 112, 111, 111, 108, 107,
  107, 102, 107, 108, 103, 100, 99, 98, 97, 95, 91, 91, 91, 88, 86, 94, 98, 103,
  103, 102, 105, 115, 116, 120, 122, 125, 127, 127, 121, 119, 116, 121, 118, 116,
  106, 106, 94, 93, 91, 87, 83, 82, 82, 82, 82, 81, 82, 82, 82, 82, 79, 81, 77, 77,
  75, 76, 80, 89, 89, 89, 99, 99, 99, 105, 106, 109, 111, 113, 114, 120, 115, 111,
  107, 97, 95, 92, 106, 128, 131, 132, 138, 139, 134, 133, 132, 131, 130, 124,
  124, 124, 122, 120, 116, 116, 113, 115, 119, 124, 124, 126, 127, 127, 127, 127,
  127, 127, 130, 131, 133, 135, 136, 134, 133, 131, 131, 131, 131, 131, 131, 130,
  130, 127, 126, 122, 122, 122, 123, 128, 130, 132, 131, 129, 126, 126, 126, 126,
  128, 128, 128, 128, 134, 134, 134, 134, 134, 133, 131, 131, 131, 131, 140, 140,
  140, 143, 141, 135, 130, 125, 128, 134, 134, 133, 133, 133, 133, 133, 133, 132,
  131, 131, 135, 138, 141, 141, 146, 142, 138, 138, 138, 148, 147, 147, 147, 144,
  144, 148, 147, 146, 143, 145, 140, 140, 145, 147, 145, 145, 144, 144, 145, 146,
  148, 141, 145, 144, 138, 138, 138, 122, 122, 122, 122, 121, 120, 130, 143, 143,
  144, 144, 144, 149, 150, 150, 150, 149, 152, 152, 150, 148, 148, 148, 148, 148
};
int BPMupdateInterval = 2000;
long lastBPMupdateTime = 0;
long BPMpos = 0;

/************************** Sketch Code **************************/
// for changing between output test modes
#define SLEEP 0
#define BEAT 1
// 2 : Red lights intensity changes with presence
// 3 : White intensity changes with presence (diferent pixels light)
// 4 : White intensity changes with presence (same pixels intensity)
#define LIGHT 4
uint32_t lastOutMode = LIGHT;
uint32_t outMode = LIGHT;


/***************************** Setup *****************************/
void setup() {
  // Initialize serial communication with computer, for debugging
  //Serial.begin(115200);

  // Declare the PIR sensor pin as an input
  pinMode(PIR_PIN, INPUT);

  // Initialize the strip object
  strip.begin();
  // Initialize all pixels to 'off'
  strip.show();
  previousLedUpdateTime = millis();

  // Mark the initial beat times
  lastBeatTime = millis();
  lastBPMupdateTime = millis();

  // Mark initial photocell time
  lastPhotoReading = millis();

}


/***************************** Loop *****************************/
void loop() {
  // Read the input on PIR sensor pin:
  int pirVal = digitalRead(PIR_PIN);
  boolean presence;
  if (pirVal == HIGH) {
    presence = true;
    lastTimeActive = millis();
  } else {
    presence = false;
  }

  if ((millis() - lastTimeActive) >= sleepInterval) {
    if (outMode != SLEEP)
      outMode = SLEEP;

  } else {
    if ((millis() - lastPhotoReading) >= photoInterval) {
      // read the resistor
      photoRead = analogRead(A2);
      //Serial.println(photoRead);

      // THRESHOLD SHOULD ADAPT TO THE LIGHTNING CONDITIONS
      if (photoRead > photoThreshold)
        outMode = BEAT;
      else
        outMode = LIGHT;

      // mark time again
      lastPhotoReading = millis();
    }
  }

  if (outMode == SLEEP)
    doSleep();
  else if (outMode == BEAT)
    doBeat();
  else
    doLight(presence);

  /*
  if ((millis() - lastVoltageCheck) > voltageCheckInterval){
    double theVoltage = batteryVoltage();
    //Serial.print("Battery Voltage: ");
    //Serial.print(theVoltage);

    if (theVoltage < 3.5)
      oB = 255;
    else if (theVoltage < 3.7)
      oG = 255;
    else
      oB = 0, oG = 0;

    lastVoltageCheck = millis();
  }
  */

  // Adding delays will prevent the NeoPixels from working smoothly,
  // use timers instead:
  // http://wiring.org.co/learning/basics/timer.html
}

void doSleep() {
  int desiredOutValueRed = 0;
  float desiredSpeedRed = 128.0;
  int desiredOutValueWhite = 0;
  float desiredSpeedWhite = 128.0;

  if ((millis() - previousLedUpdateTime) > ledUpdateInterval) {
    // Smoothly transition towards the desired value at the desired speed,
    // the higher the speed value the slower the transition
    float difRed = desiredOutValueRed - outValueRed;
    if (abs(difRed) > 1.0)
      outValueRed = outValueRed + difRed / desiredSpeedRed;
    float difWhite = desiredOutValueWhite - outValueWhite;
    if (abs(difWhite) > 1.0)
      outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

    for (uint16_t i = 0; i < strip.numPixels(); i++)
      strip.setPixelColor(i, outValueRed, oG, oB, outValueWhite);

    strip.show();

    // Mark the time
    previousLedUpdateTime = millis();
  }

  if ((millis() - lastCalibration) >= calibrationInterval
      && outValueWhite < 5) {
    isCalibrating = true;
    lastCalibration = millis();
  }

  if (isCalibrating) {
    if ((millis() - lastPhotoReading) >= photoInterval) {
      // read the resistor
      photoRead = analogRead(A2);
      calSamples.push(photoRead);
      //Serial.println(photoRead);
      samplesTaken++;

      if (samplesTaken >= SAMPLE_SIZE) {
        //Serial.print("Mode: ");
        //Serial.println(calSamples.mode());
        //Serial.print("Mean: ");
        //Serial.println(calSamples.mean());
        //Serial.print("Devi: ");
        //Serial.println(calSamples.stddev());
        photoThreshold = calSamples.mode() + 50;
        
        samplesTaken = 0;
        isCalibrating = false;
      }

      lastPhotoReading = millis();
    }
  }
}

void doLight(boolean _presence) {
  if (outMode == 2) {
    // For slow transitions
    int desiredOutValueRed = 0;
    float desiredSpeedRed = 0;
    int desiredOutValueWhite = 0;
    float desiredSpeedWhite = 0;

    // if someone is detected
    if (_presence) {
      // Set the output value to the maximum
      desiredOutValueRed = 255;
      desiredSpeedRed = 8.0;
      desiredOutValueWhite = 100;
      desiredSpeedWhite = 64.0;
    } else {
      // set the output value to the minimum
      desiredOutValueRed = 0;
      desiredSpeedRed = 128.0;
      desiredOutValueWhite = 255;
      desiredSpeedWhite = 64.0;
    }

    if ((millis() - previousLedUpdateTime) > ledUpdateInterval) {
      // Smoothly transition towards the desired value at the desired speed,
      // the higher the speed value the slower the transition
      float difRed = desiredOutValueRed - outValueRed;
      if (abs(difRed) > 1.0)
        outValueRed = outValueRed + difRed / desiredSpeedRed;
      float difWhite = desiredOutValueWhite - outValueWhite;
      if (abs(difWhite) > 1.0)
        outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

      //Serial.print(desiredOutValueRed);
      //Serial.print("\t");

      // Change the pixels out values
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if (i % 2 == 0)
          strip.setPixelColor(i, 0, oG, oB, outValueWhite);
        else
          strip.setPixelColor(i, outValueRed, oG, oB, 0);
      }
      strip.show();

      //Serial.println(outValueRed);

      // Mark the time
      previousLedUpdateTime = millis();
    }

  } else if (outMode == 3) {
    // For slow transitions
    int desiredOutValueRed = 0;
    float desiredSpeedRed = 0;
    int desiredOutValueWhite = 0;
    float desiredSpeedWhite = 0;

    // if someone is detected
    if (_presence) {
      // Set the output value to the maximum
      desiredOutValueRed = 255;
      desiredSpeedRed = 8.0;
      desiredOutValueWhite = 255;
      desiredSpeedWhite = 64.0;
    } else {
      // set the output value to the minimum
      desiredOutValueRed = 0;
      desiredSpeedRed = 128.0;
      desiredOutValueWhite = 255;
      desiredSpeedWhite = 64.0;
    }

    if ((millis() - previousLedUpdateTime) > ledUpdateInterval) {
      // Smoothly transition towards the desired value at the desired speed,
      // the higher the speed value the slower the transition
      float difRed = desiredOutValueRed - outValueRed;
      if (abs(difRed) > 1.0)
        outValueRed = outValueRed + difRed / desiredSpeedRed;
      float difWhite = desiredOutValueWhite - outValueWhite;
      if (abs(difWhite) > 1.0)
        outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

      //Serial.print(desiredOutValueRed);
      //Serial.print("\t");

      // Change the pixels out values
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if (i % 2 == 0)
          strip.setPixelColor(i, 0, oG, oB, outValueWhite);
        else
          strip.setPixelColor(i, 0, oG, oB, outValueRed);
      }
      strip.show();

      //Serial.println(outValueRed);

      // Mark the time
      previousLedUpdateTime = millis();
    }

  } else {
    // For slow transitions
    int desiredOutValueWhite = 0;
    float desiredSpeedWhite = 0;

    // if someone is detected
    if (_presence) {
      // Set the output value to the maximum
      desiredOutValueWhite = 255;
      desiredSpeedWhite = 8.0;
    } else {
      // Set the output value to the minimum
      desiredOutValueWhite = 50;
      desiredSpeedWhite = 64.0;
    }

    if ((millis() - previousLedUpdateTime) > ledUpdateInterval) {
      // Smoothly transition towards the desired value at the desired
      // speed, the higher the speed value the slower the transition
      float difWhite = desiredOutValueWhite - outValueWhite;
      if (abs(difWhite) > 1.0)
        outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

      //Serial.println(desiredOutValueWhite);

      // Change the pixels out values
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0, oG, oB, outValueWhite);
      }
      strip.show();

      // Mark the time
      previousLedUpdateTime = millis();
    }
  }
}

void doBeat() {
  if (((millis() - lastBeatTime) >= beatInterval)
      && (phase == DIASTOLE_ONE)) {

    outValueRed = 100;

    phase = DIASTOLE_TWO;
    lastPhaseTime = millis();
    lastBeatTime = millis();
  }
  else if ((millis() - lastPhaseTime >= phaseInterval)
           && (phase == DIASTOLE_TWO)) {

    outValueRed = 0;

    phase = SYSTOLE_ONE;
    lastPhaseTime = millis();
  }
  else if ((millis() - lastPhaseTime >= phaseInterval)
           && (phase == SYSTOLE_ONE)) {

    outValueRed = 100;

    phase = SYSTOLE_TWO;
    lastPhaseTime = millis();
  }
  else if ((millis() - lastPhaseTime >= phaseInterval)
           && (phase == SYSTOLE_TWO)) {

    outValueRed = 0;

    phase = DIASTOLE_ONE;
    lastPhaseTime = millis();
  }


  // Update pixels
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    if (i % 2 == 0)
      strip.setPixelColor(i, 0, oG, oB, outValueWhite);
    else
      strip.setPixelColor(i, 0, oG, oB, outValueRed);
  }
  strip.show();


  if ((millis() - lastBPMupdateTime) >= BPMupdateInterval) {
    BPMpos++;
    if (BPMpos >= sizeof(bpms) / sizeof(bpms[0]))
      BPMpos = 0;

    bpm = bpms[BPMpos];
    bpm = constrain(bpm, 1, 195);
    // calculate beat frequency in  milliseconds
    beatInterval = 60000 / bpm;
    // int for storing duration of phase intervals
    phaseInterval = ((beatInterval / 4) * 3) / 4;
    if (phaseInterval > 100) phaseInterval = 100;

    lastBPMupdateTime = millis();
  }
}

double batteryVoltage() {
  double measuredvbat = analogRead(VBAT_PIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  return measuredvbat;
}

