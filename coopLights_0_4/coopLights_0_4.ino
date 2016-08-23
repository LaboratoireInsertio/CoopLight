/* CoopLights 0.4
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
char ssid[] = "*****";
// your network password (use for WPA, or use as key for WEP)
char pass[] = "*****";
// your network key Index number (needed only for WEP)
int keyIndex = 0;

int status = WL_IDLE_STATUS;

/************************ Adafruit.io Setup ************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "*****"
#define AIO_KEY         "*****"

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
unsigned long previousCapUpdateTime;
// Update interval in milliseconds
int capUpdateInterval = 1000;

/************************ PIR Sensor Setup ************************/
// PIR sensor pin number
#define PIR_PIN 11

/**************** Battery Voltage Monitoring Setup ****************/
// Battery voltage monitoring pin number
#define VBAT_PIN 9 // Should be A7 but it apears not to be declared

// Battery voltage update timer
unsigned long previousVolUpdateTime;
// Update interval in milliseconds
int volUpdateInterval = 300000; // Every 5 minutes

/************************** Sketch Code **************************/
// for changing between output test modes
// 1 : Red lights with presence
// 2 : White intensity changes with presence
int outputMode = 1;


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
  previousCapUpdateTime = millis();


  MQTT_connect();
  // Publishes Battery Voltage to io.adafruit.com. This is repeated
  // every 5 minutes while the program is runing.
  // The right syntax to deal with floating point data with the publish 
  // method is: publish(double value, int precision) where value is the
  // value expected to be send and precision is the number of decimals.
  double measuredvbat = analogRead(VBAT_PIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  // Now we can publish stuff!
  //Serial.print(F("\nSending voltage val "));
  //Serial.print(measuredvbat);
  //Serial.print("...");
  if (! batteryVol.publish(measuredvbat, 1)) {
    Serial.println(F("Failed!"));
  } else {
    Serial.println(F("OK!"));
  }
  previousVolUpdateTime = millis();

}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make
  // the first connection and automatically reconnect when disconnected).
  // See the MQTT_connect function definition further below.
  MQTT_connect();

  cap_sensor.update();

  // Read the input on PIR sensor pin:
  int sensorValue = digitalRead(PIR_PIN);
  boolean presence;
  if (sensorValue == HIGH)
    presence = true;
  else
    presence = false;

  if (outputMode == 1) {
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

    if ((millis() - previousLedUpdateTime) > ledUpdateInterval) {
      // Smoothly transition towards the desired value at the desired speed,
      // the higher the speed value the slower the transition
      float difRed = desiredOutValueRed - outValueRed;
      if (abs(difRed) > 1.0)
        outValueRed = outValueRed + difRed / desiredSpeedRed;
      float difWhite = desiredOutValueWhite - outValueWhite;
      if (abs(difWhite) > 1.0)
        outValueWhite = outValueWhite + difWhite / desiredSpeedWhite;

      //Serial.println(desiredOutValueWhite);

      // Change the pixels out values
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        //strip.setPixelColor(i, outValueRed, 0, 0, outValueWhite);
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
    if (presence) {
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

  // Print out the value you read
  //Serial.println(presence, DEC);


  // Print out the value of the capacitance sensor
  if (cap_sensor.onChange()) {
    uint32_t valueToSend = cap_sensor.getValue();
    //Serial.println(valueToSend);
    //Serial.println(outValueWhite);

    if ((millis() - previousCapUpdateTime) > capUpdateInterval) {
      // Now we can publish stuff!
      //Serial.print(F("\nSending capacitance val "));
      //Serial.print(valueToSend);
      //Serial.print("...");
      if (! capacitance.publish(valueToSend)) {
        Serial.println(F("Capacitance Failed!"));
      } else {
        Serial.println(F("Capacitance OK!"));
      }

      previousCapUpdateTime = millis();
    }
  }

  if ((millis() - previousVolUpdateTime) > volUpdateInterval) {
    double measuredvbat = analogRead(VBAT_PIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage

    // Now we can publish stuff!
    //Serial.print(F("\nSending voltage val "));
    //Serial.print(measuredvbat);
    //Serial.print("...");
    if (! batteryVol.publish(measuredvbat, 1)) {
      Serial.println(F("Voltage Failed!"));
    } else {
      Serial.println(F("Voltage OK!"));
    }

    previousVolUpdateTime = millis();
  }

  // Adding delays will prevent the capacitance sensor library from
  // working properly! Use timers instead:
  // http://wiring.org.co/learning/basics/timer.html
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

