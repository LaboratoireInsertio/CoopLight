// variable for slow transitions
float outputValue = 0.0;

// for changing between testing modes
int testMode = 2;

int RED_LED = 9;
int WHITE_LED = 5;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A4);


  if (testMode == 1) {
    int desiredValue = 0;

    // keep one the light on
    analogWrite(WHITE_LED, 50);

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
      desiredValue = 0;

      // smoothly transition towards the max value
      float dif = desiredValue - outputValue;
      if (abs(dif) > 1.0) {
        outputValue = outputValue + dif / 64.0;
      }
    }

    // change the analog out value:
    analogWrite(RED_LED, outputValue);
    // print out the value you read:
    Serial.println(outputValue);
    delay(30);        // delay in between reads for stability
  }
  else if (testMode == 2) {
    int desiredValue = 50;

    // keep one the light on
    analogWrite(RED_LED, 0);

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
}
