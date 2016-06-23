// Tested with Arduino 1.7.10 from Arduino.org

// XBee-Arduino by Andrew Rapp (2009)
// https://github.com/andrewrapp/xbee-arduino
#include <XBee.h>
#include <Servo.h>
#include <Button.h>

// Creates the XBee library project
XBee xbee = XBee();
// Creates the Xbee response object for handling the recieved
// messages from other XBees.
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();
// Zigbee Transmit Request API packet.
ZBTxRequest txRequest;
// Array for adding data to outgoing Zigbee messages
uint8_t payload[] = {
  0
};

// The sensor pins:
// Infrared proximity sensors pointing in diferent directions
// might be needed. This code uses one connected to the A4 pin
// for testing.
const int PROXIMITY_SENSOR = A4;

// The LED pins:
// Currently looking into ultra bright LED strips using PWM 
// pins 5 and 9 for testing.
const int RED_LED = 9;
const int WHITE_LED = 5;

// Variable to compare the previous representative sensor 
// reading with the current  reading.
int lastSensorReading = 0;


// variable for slow transitions
float outputValue = 0.0;



// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A4);





  int desiredValue = 50;

  // if someone is detected
  if (sensorValue > 120) {
    // set the output value to the maximum
    desiredValue = 255;

    // smoothly transition towards the max value
    float dif = desiredValue - outputValue;
    if (abs(dif) > 1.0) {
      outputValue = outputValue + dif / 8.0;
    }
  } else {
    // set the output value to the maximum
    desiredValue = 50;

    // smoothly transition towards the max value
    float dif = desiredValue - outputValue;
    if (abs(dif) > 1.0) {
      outputValue = outputValue + dif / 64.0;
    }
  }

  // change the analog out value:
  analogWrite(WHITE_LED, outputValue);
  // print out the value you read:
  Serial.println(outputValue);
  delay(30);        // delay in between reads for stability
}

