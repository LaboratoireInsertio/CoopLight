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

// LED intensity at rest (from 0 to 255):
const int INTENSITY_AT_REST = 50;

// Variable to compare the previous significant sensor
// reading with the current reading.
int lastSensorValue = 0;
// Variable for storing the sensors treshhold.
int TRESHOLD = 300;

// Variable for smooth transitions
float realOutputValue = 0.0;


void setup() {
  // Start serial (Serial: 0 (RX) and 1 (TX). Used to receive
  // (RX) and transmit (TX) TTL serial data using the ATmega32U4
  // hardware serial capability. Note that on the Leonardo, the
  // Serial class refers to USB (CDC) communication; for TTL
  // serial on pins 0 and 1, use the Serial1 class.

  // For serial USB communication with computer and debuging
  Serial.begin(57600);
  // For serial communication with XBee
  Serial1.begin(57600);
  xbee.setSerial(Serial1);

  // Prepare the Zigbee Transmit Request API packet
  // Set the paylod for the data to be sent
  txRequest.setPayload(payload, sizeof(payload));
  // Identifies the UART data frame for the host to correlate
  // with  subsequent ACK (acknowledgment). If set to 0, no
  // response is sent.
  txRequest.setFrameId(0);
  // Disable ACK (acknowledgement)
  txRequest.setOption(1);

  // Pin functionality
  pinMode(PROXIMITY_SENSOR, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
}


void loop() {
  // Read the input on the proximity sensor:
  int sensorValue = analogRead(PROXIMITY_SENSOR);

  // Send the sensor value to the other lamps if there has been
  // a significant change between readings.
  if (abs(sensorValue - lastSensorValue) > 60) {
    //sendPacket(XBeeAddress64(0x0013a200, 0x40e66da0), sensorValue);
    //sendPacket(XBeeAddress64(0x0013a200, 0x40e66dcd), sensorValue);
    //sendPacket(XBeeAddress64(0x0013a200, 0x40e66c35), sensorValue);
    //sendPacket(XBeeAddress64(0x0013a200, 0x40e66dca), sensorValue);
    //sendPacket(XBeeAddress64(0x0013a200, 0x40e66c1b), sensorValue);
    
    Serial.println(sensorValue);
    
    lastSensorValue = sensorValue;
  }
  
  // Reads messages from the other lamps.
  xbee.readPacket();
  // If there 
  if (xbee.getResponse().isAvailable()) {
    // got something
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      // got a zb rx packet

      // now fill our zb rx class
      xbee.getResponse().getZBRxResponse(rx);

      // get value of the first byte in the data
      //dataIn = rx.getData(0);
      // Do something with dataIn (dataIn was taken from elseware)
    }
  }

  // If precense is detected.
  if (sensorValue > TRESHOLD) {
    // Set the desired output value to the maximum.
    int desiredOutputValue = 255;

    // Transition smoothly and fast towards the desired
    // output value.
    float dif = desiredOutputValue - realOutputValue;
    if (abs(dif) > 1.0)
      realOutputValue = realOutputValue + dif / 8.0;

  } else {
    // Set the desired output value to the desired output
    // at rest.
    int desiredOutputValue = INTENSITY_AT_REST;

    // Transition smoothly and slow towards the desired
    // output value.
    float dif = desiredOutputValue - realOutputValue;
    if (abs(dif) > 1.0)
      realOutputValue = realOutputValue + dif / 64.0;

  }


  // TODO: Update the TRESHOLD depending on the environment,
  // if the object changes place the distance from the fixed
  // objects might change.

  // TODO: Smooth sensor readings:
  // http://wiring.org.co/learning/basics/smooth.html


  // Write the new value on the given pin.
  analogWrite(WHITE_LED, realOutputValue);
  // Print out the output value.
  //Serial.println(realOutputValue);
  // Delay in between reads for stability.
  delay(20);
}


void sendPacket(XBeeAddress64 addr64, uint8_t val) {
  // Set the destination address of the message
  txRequest.setAddress64(addr64);
  payload[0] = val;
  // Send the message
  xbee.send(txRequest);
}
