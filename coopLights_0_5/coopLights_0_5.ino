///////////////// CLEAR SENSITIVE DATA BEFORE COMMIT /////////////////
///////////////// CLEAR SENSITIVE DATA BEFORE COMMIT /////////////////
///////////////// CLEAR SENSITIVE DATA BEFORE COMMIT /////////////////
///////////////// CLEAR SENSITIVE DATA BEFORE COMMIT /////////////////

/* CoopLights 0.5
   Developed and tested on Arduino 1.6.10 (CC)

   0.2: Uses the Feather M0 WIFI instead of the Feather HUZZAH due to
   problems of compatibility with capacitance sensing.
   Implements capacitance sensing.
   Abandons IR Distance Sensor implementation (Sticks with PIR).

   0.3: Adds WIFI support.
   Uploads capacitance sensor readings to io.adafruit.com.

   0.4: Deals with the NeoPixel Feather Wing overheating issue by
   using only half of the pixels in the Wing.
   Implements battery voltage monitoring trough io.adafruit.com.
   This version appears to be very power inefficient probably due
   to the use of WIFI. More testing needs to be done. WIFI sleeping
   capabilities should be implemented.

   0.5: Changes the PIR sensor pin to A5 (Looks like the capacitance
   sensor was provoking some interference since there where next to
   each other).
   Detects historica changes in capacitance redings.
   Implements pre-recorder heath beating.
   Transitions between modes need to be smoothed.


   • For instruction on how to get the Feather M0 WIFI running check:
   https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/
   WARNING: When using the WIFI functionalities be careful not to
   upload private usernames or passwords to the GitHub!
   • And this on the io.adafruit.com service:
   https://learn.adafruit.com/mqtt-adafruit-io-and-you/
   https://learn.adafruit.com/adafruit-io/getting-started/
   WARNING: io.adafruit.com is a free service that allows a maximum
   exchange of two values per second per account!
   • For more information on the NeoPixel Feather Wing check:
   https://learn.adafruit.com/adafruit-neopixel-featherwing/
   WARNING: Turning all the 36 pixels of the Wing at maximum brightness
   at the same time, pulls more current than what the battery is
   able to provide and overheats the circuits. Doing this for a long
   time causes malfunctioning and eventually might burn the components!
   • For more on the Capacitance sensing library check (Not every pin
   in the Feather M0 WIFI works as a receive pin!):
   http://robotsbigdata.com/docs-arduino-capacitance.html
   • And this on the Adafruit MPR121 Capacitive Touch Sensor (that
   we're not using):
   https://forums.adafruit.com/viewtopic.php?f=19&t=72025
*/
#include <SPI.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_WINC1500.h>
#include <Adafruit_NeoPixel.h>
#include <RBD_Capacitance.h>

/**************************** WiFI Setup ****************************/
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2

Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

// your network SSID (name)
char ssid[] = "***";
// your network password (use for WPA, or use as key for WEP)
char pass[] = "***";
// your network key Index number (needed only for WEP)
int keyIndex = 0;

int status = WL_IDLE_STATUS;

/************************ Adafruit.io Setup ************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "***"
#define AIO_KEY         "***"

/********** Global State (you don't need to change this!) **********/
//Set up the wifi client
Adafruit_WINC1500Client client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

/****************************** Feeds ******************************/
// Setup the feeds on io.adafruit.com for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish capacitance = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/cooplight-capacitancetest");
Adafruit_MQTT_Publish batteryVol = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/cooplight-batteryvoltagetest");

/************************* NeoPixel Setup *************************/
// NeoPixel pin number
#define NEO_PIN 6
// Number of LEDs in the NeoPixel strip, ring, etc
#define NEO_NUM 32
// Type of NeoPixel
//  NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//  NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//  NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//  NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//  NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
//  NEO_GRBW    Pixels are wired for GRBW bitstream (NeoPixel Jewel)
#define NEO_TYPE NEO_GRB + NEO_KHZ800

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

/******************** Capacitance Sensor Setup ********************/
// Capacitance send and recieve pin numbers
#define CAPS_PIN 12
#define CAPR_PIN 5

// Parameter 1 = Send pin
// Parameter 2 = Recieve pin
RBD::Capacitance cap_sensor(CAPS_PIN, CAPR_PIN);

// Capacitance sensor update timer
unsigned long lastCapUpdateTime;
// Update interval in milliseconds
int capUpInterval = 1000;

/******************* Capacitance Interpretation *******************/
#include <Average.h>

#define CAP_SAM_SIZE 50
Average<uint16_t> capSamples(CAP_SAM_SIZE);

/************************ PIR Sensor Setup ************************/
// PIR sensor pin number
#define PIR_PIN A5

/**************** Battery Voltage Monitoring Setup ****************/
// Battery voltage monitoring pin number
#define VBAT_PIN 9 // Should be A7 but it apears not to be declared

// Battery voltage update timer
unsigned long lastVolUpdateTime;
// Update interval in milliseconds
int volUpInterval = 300000; // Every 5 minutes

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
#define BEAT 0
// 1 : Red lights intensity changes with presence
// 2 : White intensity changes with presence
#define LIGHT 1
uint8_t outMode = LIGHT;


/***************************** Setup *****************************/
void setup() {
#ifdef WINC_EN
  pinMode(WINC_EN, OUTPUT);
  digitalWrite(WINC_EN, HIGH);
#endif

  // Initialize serial communication with computer, for debugging
  //while (!Serial);
  Serial.begin(115200);

  // Initialise the Client
  Serial.print(F("\nInit the WiFi module..."));
  // check for the presence of the breakout
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WINC1500 not present");
    // don't continue:
    while (true);
  }
  Serial.println("ATWINC OK!");

  // Declare the PIR sensor pin as an input
  pinMode(PIR_PIN, INPUT);

  // Initialize the strip object
  strip.begin();
  // Initialize all pixels to 'off'
  strip.show();
  previousLedUpdateTime = millis();

  // Reduce the variability of the capacitance sensor readings.
  // Default valut is 100.
  cap_sensor.setSampleSize(1000);
  lastCapUpdateTime = millis();


  MQTT_connect();
  // Publishes Battery Voltage to io.adafruit.com. This is repeated
  // every 5 minutes while the program is runing.
  // The right syntax to deal with floating point data with the publish
  // method is: publish(double value, int precision) where value is the
  // value expected to be send and precision is the number of decimals.
  double measuredvbat = batteryVoltage();
  if (! batteryVol.publish(measuredvbat, 1));
  //Serial.println(F("Failed!"));
  //else
  //Serial.println(F("OK!"));

  lastVolUpdateTime = millis();

  // Mark the initial beat times
  lastBeatTime = millis();
  lastBPMupdateTime = millis();

  // Fills samples array with 0's
  for (int i = 0; i < CAP_SAM_SIZE; i++)
    capSamples.push(0);

}


/***************************** Loop *****************************/
void loop() {
  // Ensure the connection to the MQTT server is alive (this will make
  // the first connection and automatically reconnect when disconnected).
  // See the MQTT_connect function definition further below.
  MQTT_connect();

  // Read the input on PIR sensor pin:
  int pirVal = digitalRead(PIR_PIN);
  boolean presence;
  if (pirVal == HIGH)
    presence = true;
  else
    presence = false;

  // Read the capacitance sensor
  cap_sensor.update();
  int32_t capVal;
  if (cap_sensor.onChange()) {
    capVal = cap_sensor.getValue();
    // Publish the value of the capacitance sensor with the given interval
    lastCapUpdateTime = capTimedPublish(capVal, capUpInterval, lastCapUpdateTime);


    capSamples.push(capVal);

    // Display the current data set
    for (int i = 0; i < CAP_SAM_SIZE; i++) {
      Serial.print(capSamples.get(i));
      Serial.print(" ");
    }
    Serial.println();
    Serial.print("Mean:   "); Serial.println(capSamples.mean());
    Serial.print("Mode:   "); Serial.println(capSamples.mode());
    Serial.print("StdDev: "); Serial.println(capSamples.stddev());

    // Average of the las 3 readings
    int avgLatest = 0;
    for (int i = CAP_SAM_SIZE-3; i < CAP_SAM_SIZE; i++)
      avgLatest += capSamples.get(i);
    avgLatest /= 3;
    Serial.print("Dif:    "); Serial.println(abs(avgLatest - capSamples.mode()));
    if (abs(avgLatest - capSamples.mode()) > 40)
      outMode = BEAT;
    else
      outMode = LIGHT;
  }



  if (outMode == BEAT) {
    doBeat();
  } else {
    doLight(presence);
  }



  // Publish the voltage of the battery with the given interval
  double bVol = batteryVoltage();
  lastVolUpdateTime = volTimedPublish(bVol, volUpInterval, lastVolUpdateTime);

  // Adding delays will prevent the capacitance sensor library from
  // working properly! Use timers instead:
  // http://wiring.org.co/learning/basics/timer.html
}


uint32_t volTimedPublish (double _value, uint16_t _interval, uint32_t _lastPublish) {
  if ((millis() - _lastPublish) > _interval) {
    if (! batteryVol.publish(_value, 1));
    //Serial.println(F("Publish Failed!"));
    //else
    //Serial.println(F("Publish OK!"));

    _lastPublish = millis();
  }

  return _lastPublish;
}

uint32_t capTimedPublish (uint32_t _value, uint16_t _interval, uint32_t _lastPublish) {
  if ((millis() - _lastPublish) > _interval) {
    if (! capacitance.publish(_value));
    //Serial.println(F("Publish Failed!"));
    //else
    //Serial.println(F("Publish OK!"));

    _lastPublish = millis();
  }

  return _lastPublish;
}

double batteryVoltage() {
  double measuredvbat = analogRead(VBAT_PIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  return measuredvbat;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

void doLight(boolean _presence) {
  if (outMode == 1) {
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
      desiredSpeedRed = 64.0;
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
          strip.setPixelColor(i, outValueWhite, outValueWhite, outValueWhite);
        else
          strip.setPixelColor(i, outValueRed, 0, 0);
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
        if (i % 2 == 0)
          strip.setPixelColor(i, outValueWhite, outValueWhite, outValueWhite);
        else
          strip.setPixelColor(i, 0, 0, 0);
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
    // Turn all pixels off
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      if (i % 2 == 0)
        strip.setPixelColor(i, 255, 255, 255);
      else
        strip.setPixelColor(i, 100, 100, 100);
    }
    strip.show();

    phase = DIASTOLE_TWO;
    lastPhaseTime = millis();
    lastBeatTime = millis();
  }
  else if ((millis() - lastPhaseTime >= phaseInterval)
           && (phase == DIASTOLE_TWO)) {
    // Turn all pixels on
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      if (i % 2 == 0)
        strip.setPixelColor(i, 255, 255, 255);
      else
        strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();

    phase = SYSTOLE_ONE;
    lastPhaseTime = millis();
  }
  else if ((millis() - lastPhaseTime >= phaseInterval)
           && (phase == SYSTOLE_ONE)) {
    // Turn all pixels off
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      if (i % 2 == 0)
        strip.setPixelColor(i, 255, 255, 255);
      else
        strip.setPixelColor(i, 100, 100, 100);
    }
    strip.show();

    phase = SYSTOLE_TWO;
    lastPhaseTime = millis();
  }
  else if ((millis() - lastPhaseTime >= phaseInterval)
           && (phase == SYSTOLE_TWO)) {
    // Turn all pixels on
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      if (i % 2 == 0)
        strip.setPixelColor(i, 255, 255, 255);
      else
        strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();

    phase = DIASTOLE_ONE;
    lastPhaseTime = millis();
  }


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


