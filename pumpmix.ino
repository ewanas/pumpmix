/**
 * Simple program that receives commands over serial to control a pump, servo
 * depending on values from an FSR.
 *
 * The commands are as follows:
 *  MAX <int>
 *    Sets the maximum value to be read from the FSR that the pump should stop
 *    to operate at. Any value above that, and if the pump is in AUTO mode,
 *    will stop the pump.
 *  MIN <int>
 *    Sets the minimum value to be read from the FSR that the Servo should start
 *    to operate at. Any value below that, and if the servo is in AUTO mode,
 *    the servo will be stopped.
 *  PUMP <ON|OFF|AUTO>
 *    If AUTO, pump is on, if pressure is <= MAX
 *    Otherwise, according to ON, OFF
 *  SERVO <ON|OFF|AUTO>
 *    If AUTO, servo will turn on, if pressure is >= MIN
 *    Otherwise, according to ON, OFF
 *  STATE
 *    Generates a response in the following format
 *      (ON|OFF|AUTO);(ON|OFF|AUTO);FSR;MIN;MAX;
 *      FSR is a floating point which denotes the force in Newtons
 *      MIN is an integer which denotes the current minimum
 *      MAX is an integer which denotes the current maximum
 */

#define PUMP  4
#define SERVO 3
#define FSR   A0

#define OFF   0
#define ON    1
#define AUTO  2

int startsWith (char* str, char* value) {
  return !(strncmp (str, value, strlen (value)));
}

struct {
  int state;
} Pump;

struct {
  int state;
} Servo;

struct {
  int   reading;
  float value;
} Fsr;

struct {
  int minimum;
  int maximum;
} Range;

void setup () {
  pinMode (PUMP, OUTPUT);
  pinMode (SERVO, OUTPUT);
  pinMode (FSR, INPUT);

  Servo.state = Pump.state = AUTO;
  Range.minimum = 400;
  Range.maximum = 800l;

  Serial.begin (9600);
}

void pumpCmd (char* cmd) {
  if (startsWith (cmd, "ON")) {
    Pump.state = ON;
  } else if (startsWith (cmd, "OFF")) {
    Pump.state = OFF;
  } else if (startsWith (cmd, "AUTO")) {
    Pump.state = AUTO;
  } else {
    errorCmd ("Pump error, invalid value", cmd);
  }

  stateCmd ();
}

void servoCmd (char* cmd) {
  if (startsWith (cmd, "ON")) {
    Servo.state = ON;
  } else if (startsWith (cmd, "OFF")) {
    Servo.state = OFF;
  } else if (startsWith (cmd, "AUTO")) {
    Servo.state = AUTO;
  } else {
    errorCmd ("Servo error, invalid value", cmd);
  }

  stateCmd ();
}

void maxCmd (char* cmd) {
  int tmp;
  sscanf (cmd, "%d", &tmp);

  if (tmp < Range.minimum) {
    errorCmd ("Max error, value less than minimum", cmd);
  } else {
    Range.maximum = tmp;
  }

  stateCmd ();
}

void minCmd (char* cmd) {
  int tmp;
  sscanf (cmd, "%d", &tmp);

  if (tmp > Range.maximum) {
    errorCmd ("Min error, value greater than maximum", cmd);
  } else {
    Range.minimum = tmp;
  }

  stateCmd ();
}

void stateCmd () {
  if (Pump.state == ON) {
    Serial.print ("ON;");
  } else if (Pump.state == OFF) {
    Serial.print ("OFF;");
  } else {
    Serial.print ("AUTO;");
  }

  if (Servo.state == ON) {
    Serial.print ("ON;");
  } else if (Servo.state == OFF) {
    Serial.print ("OFF;");
  } else {
    Serial.print ("AUTO;");
  }

  Serial.print (Fsr.value, 3);
  Serial.print (";");
  Serial.print (Range.minimum);
  Serial.print (";");
  Serial.print (Range.maximum);
  Serial.print (";");
  Serial.print ("\n");
}

void errorCmd (char* errorType, char* cmd) {
  Serial.print ("ERROR: ");
  Serial.print (errorType);
  Serial.print(": ");
  Serial.print (cmd);
  Serial.print ("\n");
}

void readInput () {
  static const int BUF_LENGTH = 256;
  static char buffer [BUF_LENGTH];

  if (Serial.available () > 0) {
    Serial.readBytesUntil ('\n', buffer, BUF_LENGTH);

    if (startsWith (buffer, "PUMP")) {
      pumpCmd (buffer + 5);
    } else if (startsWith (buffer, "SERVO")) {
      servoCmd (buffer + 6);
    } else if (startsWith (buffer, "MAX")) {
      maxCmd (buffer + 4);
    } else if (startsWith (buffer, "MIN")) {
      minCmd (buffer + 4);
    } else if (startsWith (buffer, "STATE")) {
      stateCmd ();
    } else {
      errorCmd("Invalid command", buffer);
    }

    for (int i = 0; i < BUF_LENGTH; i++) {
      buffer[i] = '\0';
    }
  }
}

void readFsr () {
  Fsr.reading = analogRead (FSR);
  Fsr.value = (float)(Fsr.reading);
}

void servo () {
  if (Servo.state == AUTO) {
    if (Fsr.value > Range.minimum) {
      analogWrite (SERVO, 128);
    } else {
      analogWrite (SERVO, 255);
    }
  } else if (Servo.state == ON) {
    analogWrite (SERVO, 128);
  } else {
    analogWrite (SERVO, 255);
  }
}

void pump () {
  if (Pump.state == AUTO) {
    if (Fsr.value > Range.maximum) {
      digitalWrite (PUMP, 0);
    } else {
      digitalWrite (PUMP, 1);
    }
  } else if (Pump.state == ON) {
    digitalWrite (PUMP, 1);
  } else {
    digitalWrite (PUMP, 0);
  }
}

void loop () {
  readInput ();

  readFsr ();

  servo ();
  pump ();
 }
