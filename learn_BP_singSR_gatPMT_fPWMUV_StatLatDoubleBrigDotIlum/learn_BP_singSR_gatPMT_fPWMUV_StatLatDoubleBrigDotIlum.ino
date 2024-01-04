// Stimulus Controller
// Serial commands accepted
//* STIM CSUS L/R X P Y N
//   CSUS - CS/US (0- only CS, 1- paired CS US, 2- only US),
//   L/R - CS on the L or R side (0- left, 1- right, 2- left and right simultaneously, 3- no CS),
//   X - duration of US (ms)
//   P - power of US
//   Y - latency of US onset relative to CS onset (ms)
//   N - number of repetitions
//* BREAK -> Stops immediately everything and return lights to default state.
//* OFF -> Immediately turn off all lights.

// May change this in the future. It's to control the power of the CS LEDs with a (slow) PWM
// const int powerCS = (255 - 100) / 255;


// ! To work for backward conditioning, IsCSOff would need to be like triggerUS, having more than 2 states.
// ! With paired stim, still not working for USLatency < delayGatingPMTBefPulse.


char inputArray[240];
String inputString = "";
boolean stringComplete = false;
boolean IsNewRep = true;

boolean IsCSOff = true;
// boolean LightsOff = false;

boolean IsPMTOff = false;
// boolean PMTOff = true;


const int delayGatingPMTBefPulse = 350;  // ms
const int delayGatingPMTAftPulse = 100;    // ms
// Time each white CS LED is ON
const int CSDuration = 10000;
const int mainPWMCycle = 999;

const int USPin = 9;
const int dataPin = 7;
const int clockPin = 5;
const int latchPin = 6;
const int outputEnablePin = 3;
const int PMTGatingPin = 4;
const int startStopAcq = 13;
// unsigned int CSUS_;
// unsigned int CSUS = 4;
unsigned int CSUS;
unsigned int LorR;
unsigned int currentStatus = 0;
unsigned int oldStatus = 0;
unsigned int triggerUS = 0;
unsigned int USPower;
unsigned int USpower;
unsigned int USLatency;
unsigned int PMTLatency;
// unsigned int numberReps;s

unsigned int currentRep = 0;
unsigned int reinforcer = 0;
unsigned long USDuration;
// unsigned long CSOnset;
unsigned long timerCS;
//  = CSDuration + 1; // Starts at a larger value than CSDuration so that a BREAK at the beginning does not output "E"
unsigned long timerPMT;
unsigned long timerUS;
// unsigned long timerUS;

//#define DEBUG 0

void setup() {
  pinMode(PMTGatingPin, OUTPUT);
  inputString.reserve(240);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(outputEnablePin, OUTPUT);
  delay(10);
  pinMode(USPin, OUTPUT);
  pinMode(10, OUTPUT);
  Serial.begin(115200);
  DDRB = 0x07;  // set OC1A pin output (among others)
  PORTB = 0;
  TCNT1 = 0;  // clear counter
  ICR1 = mainPWMCycle;
  TCCR1A = 0b10000010;   // non-inverting, fast PWM
  TCCR1B = 0b00011001;   // fast PWM, full speed
  OCR1A = mainPWMCycle;  // 1 % strobe
  // switchLightsOff();
  finishCS();
  switchPMT(true);
}

void loop() {
  if (stringComplete) {
    // Cleans input and separates in spaces
    inputString.trim();
    inputString.toCharArray(inputArray, 240);
    char *command = strtok(inputArray, " ");

    if (strcmp(command, "STIM") == 0) {
      if (currentStatus != 1) {
        command = strtok(0, " ");
        CSUS = atof(command);
        command = strtok(0, " ");
        LorR = atof(command);
        command = strtok(0, " ");
        USDuration = atof(command);
        command = strtok(0, " ");
        USpower = atof(command);
        command = strtok(0, " ");
        USLatency = atof(command);

        USPower = generateCycle(USpower);

        currentStatus = 1;
      } else {
        command = strtok(0, " ");

        reinforcer = atof(command);

        if (reinforcer == 2) {
          command = strtok(0, " ");
          command = strtok(0, " ");
          USDuration = atof(command);
          command = strtok(0, " ");
          USpower = atof(command);

          USLatency = delayGatingPMTBefPulse;
          PMTLatency = 0;

          USPower = generateCycle(USpower);

          triggerUS = 1;
        }
      }
    }

    else if (strcmp(command, "BREAK") == 0) {
      currentStatus = 0;
      oldStatus = 1;
      // LightsOff = false;
      // PMTOff = false;
    }

    else if (strcmp(command, "OFF") == 0) {
      currentStatus = 2;
      // oldStatus = 1;
      // LightsOff = true;
      // PMTOff = true;
      // if (LightsOff)
      // {
      // switchLightsOff();
      // LightsOff = false;
      // }
    }

    else if (strcmp(command, "STARTACQUISITION") == 0) {
      digitalWrite(startStopAcq, HIGH);
    }

    else if (strcmp(command, "STOPACQUISITION") == 0) {
      digitalWrite(startStopAcq, LOW);
    }

    // Clean variables used to receive serial data.
    inputString = "";
    stringComplete = false;
    memset(&inputArray[0], 0, sizeof(inputArray));
  }

  // LED STATUS ZONE AND CONTROL
  switch (currentStatus) {

      //* BREAK or end of stim.
    case 0:
      if (oldStatus != 0) {
        finishCS();
        finishUS();

        IsNewRep = true;

        triggerUS = 0;

        oldStatus = 0;
      }

      if (IsPMTOff) {
        //* Switch on the PMT.
        if (millis() - timerUS > delayGatingPMTAftPulse) {
          switchPMT(true);
        }
      }
      break;

      //* OFF
    case 2:
      if (oldStatus != 2) {
        switchLightsOff();

        IsNewRep = true;

        triggerUS = 0;

        oldStatus = 2;
      }

      break;
      //* STIM
    case 1:

      if (oldStatus != 1) {

        if (IsNewRep) {
          if (CSUS != 0) {
            triggerUS = 1;

            if (CSUS == 1) {
              PMTLatency = USLatency - delayGatingPMTBefPulse;
            }

            else if (CSUS == 2) {
              USLatency = delayGatingPMTBefPulse;
              PMTLatency = 0;
            }
          }

          if (CSUS != 2) {
            startCS(LorR);
          }

          IsNewRep = false;
        }


        //* If CS is going on and it is time to turn if off, turn it off.
        if (!IsCSOff) {
          if (millis() - timerCS >= CSDuration) {
            finishCS();
          }
        }

        if (!IsPMTOff) {
          if (triggerUS == 1) {
            if (CSUS == 0 || (CSUS == 1 && millis() - timerCS >= PMTLatency) || CSUS == 2) {
              //* Switch off the PMT if US is still to happen (triggerUS==1) and it is the right time for it.
              switchPMT(false);
            }
          }
        }

        if (IsPMTOff) {
          if (triggerUS == 1) {

            //* Switch on the US.
            if ((CSUS == 0 && millis() - timerPMT >= USLatency) || (CSUS == 1 && millis() - timerCS >= USLatency) || (CSUS == 2 && millis() - timerPMT >= USLatency)) {
              startUS();
            }
          }

          if (triggerUS == 2) {

            //* Switch off the US LED.
            if (millis() - timerUS >= USDuration) {
              finishUS();
            }
          }

          if (triggerUS == 3) {
            //* Switch on the PMT.
            if (millis() - timerUS > delayGatingPMTAftPulse) {
              switchPMT(true);
            }
          }
        }


        // * Once all stimuli are done, reset.
        if (IsCSOff && (triggerUS == 0 || triggerUS == 3) && !IsPMTOff) {
          currentStatus = 0;
          oldStatus = 1;
          // IsNewRep = true;
        }

        break;
      }
  }
}

// Waits for serial data, and it's called everytime new data comes
// Full commands always end in '\n'
void serialEvent() {
  while (Serial.available()) {
    if (!stringComplete) {
      char inChar = (char)Serial.read();
      inputString += inChar;
      if (inChar == '\n')
        stringComplete = true;
    }
  }
}

int generateCycle(float input) {
  return (int)(min(mainPWMCycle, mainPWMCycle - input * 10 + 1));
}


void startCS(boolean LorR) {
  digitalWrite(latchPin, LOW);

  timerCS = millis();

  if (currentRep == 0) {
    Serial.println("B");
  }

  switch (LorR) {
    case 0:  //* Left side
      shiftOut(dataPin, clockPin, LSBFIRST, 0b01100000);
      break;
    case 1:  //* Right side
      shiftOut(dataPin, clockPin, LSBFIRST, 0b00000110);
      break;
  }

  digitalWrite(latchPin, HIGH);

  IsCSOff = false;
}

void finishCS() {
  analogWrite(outputEnablePin, 0);  // OE (Output Enable) pin is always at 0 V, meaning that the ouput pins are always enabled.
  digitalWrite(latchPin, LOW);

  shiftOut(dataPin, clockPin, LSBFIRST, 0b00011000);
  // switchPMT(true);

  digitalWrite(latchPin, HIGH);
  analogWrite(outputEnablePin, 0);

  if (!IsCSOff) {
    Serial.println("E");
    IsCSOff = true;
  }
}


void switchLightsOff() {

  switchPMT(false);

  analogWrite(outputEnablePin, 0);  // OE (Output Enable) pin is always at 0 V, meaning that the ouput pins are always enabled.
  digitalWrite(latchPin, LOW);

  //! Here could use the SRCLR (Shift Register Clear) pin, but this pin is connected to VCC.
  shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);

  digitalWrite(latchPin, HIGH);
  analogWrite(outputEnablePin, 0);


  if (!IsCSOff) {
    Serial.println("E");
    IsCSOff = true;
  }

  // triggerUS == 2 means the US is going on.
  // if (triggerUS == 2)
  // {
  finishUS();
  // delay(delayGatingPMTAftPulse);
  // switchPMT(true);
  // }
}




void startUS() {
  timerUS = millis();

  Serial.println("b");

  OCR1A = USPower;

  triggerUS = 2;  // triggerUS == 2 means that the US LED is blinking.
}

void finishUS() {
  OCR1A = mainPWMCycle;

  if (triggerUS == 2) {
    Serial.println("e");

    // timerUS counts time after the US offset.
    timerUS = millis();

    triggerUS = 3;
  }
}



void switchPMT(boolean PMTOn) {
  if (PMTOn) {
    digitalWrite(PMTGatingPin, HIGH);
    // Serial.println("P");

    IsPMTOff = false;
  } else {
    digitalWrite(PMTGatingPin, LOW);
    // Serial.println("p");

    timerPMT = millis();
    IsPMTOff = true;
  }
}

// int powerLEDCS(float powerCS)
//{
//   return (int)((255 - powerCS) / 255);
// }