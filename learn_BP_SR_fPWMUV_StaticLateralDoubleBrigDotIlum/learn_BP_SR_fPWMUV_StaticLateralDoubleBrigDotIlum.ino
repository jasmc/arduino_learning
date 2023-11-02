// Stimulus Controller
// Serial commands accepted
//* STIM CSUS L/R X P Y N
// -> CS/US (0- only CS, 1- paired CS US, 2- only US), CS on the L or R side
//(0- left, 1- right, 2- left and right simultaneously, 3- no CS), US for X ms with power P
// after Y ms since the CS onset, number of repetitions N
//* BREAK -> Stops immediately everything and return lights to default state.
//* OFF -> Immediately turn off all lights.

// May change this in the future. It's to control the power of the CS LEDs with a (slow) PWM
// const int powerCS = (255 - 100) / 255;
char inputArray[240];
String inputString = "";
boolean stringComplete = false;
boolean IsNewRep = true;
// boolean trialBeg = true;
boolean IsCSOff = false;
boolean LightsOff = false;
// Time each white CS LED is ON
const int delay = 10000;
const int mainPWMCycle = 999;
const int dataPin = 7;
const int clockPin = 5;
const int latchPin = 6;
const int outputEnablePin = 3;
const int PMTGatingPin = 4;
// unsigned int CSUS_;
// unsigned int CSUS = 4;
unsigned int CSUS;
unsigned int LorR;
unsigned int currentStatus = 0;
unsigned int oldStatus = 0;
unsigned int triggerUV = 0;
unsigned int pulsePower;
unsigned int pulsepower;
unsigned int pulseLatency;
unsigned int numberReps;
// unsigned int j = 1;
// unsigned int n = 1;
unsigned int currentRep = 0;
unsigned int reinforcer = 0;
unsigned long USDuration;
unsigned long timer;
unsigned long timer_;
unsigned long CSOnset;
unsigned long timerUV;

//#define DEBUG 0

void setup()
{
	pinMode(PMTGatingPin, OUTPUT);
	digitalWrite(PMTGatingPin, LOW);
	inputString.reserve(240);
	pinMode(dataPin, OUTPUT);
	pinMode(clockPin, OUTPUT);
	pinMode(latchPin, OUTPUT);
	pinMode(outputEnablePin, OUTPUT);
	// cleanChip(realClean);
	delay(10);
	pinMode(9, OUTPUT);
	pinMode(10, OUTPUT);
	Serial.begin(115200);
	DDRB = 0x07; // set OC1A pin output (among others)
	PORTB = 0;
	TCNT1 = 0; // clear counter
	ICR1 = mainPWMCycle;
	TCCR1A = 0b10000010;  // non-inverting, fast PWM
	TCCR1B = 0b00011001;  // fast PWM, full speed
	OCR1A = mainPWMCycle; // 1 % strobe
	finishCS(LightsOff);
}

void loop()
{
	if (stringComplete)
	{
		// Cleans input and separates in spaces
		inputString.trim();
		inputString.toCharArray(inputArray, 240);
		char *command = strtok(inputArray, " ");

//! IF WE ALLOW CS TO START WHILE THE US IS GOING ON, NEEDS TO BE DEBUGGED!!!
		// Checks commands and changes status
		// if (strcmp(command, "STIM") == 0)
		// {
		// 	// if (currentStatus != 1)
		// 	// {
		// 	command = strtok(0, " ");
		// 	CSUS_ = atof(command);

		// 	if (currentStatus != 1 || (currentStatus == 1 && ((CSUS == 0 && CSUS_ == 2) || (CSUS == 2 && CSUS_ == 0))))
		// 		{
		// 			Serial.println(CSUS_);
		// 			CSUS = CSUS_;

		// 			command = strtok(0, " ");
		// 			LorR = atof(command);
		// 			command = strtok(0, " ");
		// 			pulseDuration = atof(command);
		// 			command = strtok(0, " ");
		// 			pulsepower = atof(command);
		// 			command = strtok(0, " ");
		// 			pulseLatency = atof(command);
		// 			command = strtok(0, " ");
		// 			numberReps = atof(command);

		// 			pulsePower = generateCycle(pulsepower);

		// 			currentStatus = 1;
		// 		}
		// 	else
		// 	{
		// 		command = strtok(0, " ");

		// 		reinforcer = atof(command);

		// 		if (reinforcer == 2)
		// 		{
		// 			command = strtok(0, " ");
		// 			command = strtok(0, " ");
		// 			pulseDuration = atof(command);
		// 			command = strtok(0, " ");
		// 			pulsepower = atof(command);

		// 			pulsePower = generateCycle(pulsepower);

		// 			triggerUV = 1;
		// 		}
		// 	}
		// }
		
		if (strcmp(command, "STIM") == 0)
		{
			if (currentStatus != 1)
			{
				command = strtok(0, " ");
				CSUS = atof(command);
				command = strtok(0, " ");
				LorR = atof(command);
				command = strtok(0, " ");
				USDuration = atof(command);
				command = strtok(0, " ");
				pulsepower = atof(command);
				command = strtok(0, " ");
				pulseLatency = atof(command);
				command = strtok(0, " ");
				numberReps = atof(command);

				pulsePower = generateCycle(pulsepower);

				currentStatus = 1;
			}
			else
			{
				command = strtok(0, " ");

				reinforcer = atof(command);

				if (reinforcer == 2)
				{
					command = strtok(0, " ");
					command = strtok(0, " ");
					USDuration = atof(command);
					command = strtok(0, " ");
					pulsepower = atof(command);

					pulsePower = generateCycle(pulsepower);

					triggerUV = 1;
				}
			}
		}

		else if (strcmp(command, "BREAK") == 0)
		{
			currentStatus = 0;
			oldStatus = 1;
			LightsOff = false;
		}

		else if (strcmp(command, "OFF") == 0)
		{
			currentStatus = 0;
			oldStatus = 1;
			LightsOff = true;
		}

		else if (strcmp(command, "STARTACQUISITION") == 0)
		{
			digitalWrite(PMTGatingPin, HIGH);
		}

		else if (strcmp(command, "STOPACQUISITION") == 0)
		{
			digitalWrite(PMTGatingPin, LOW);
		}

		// Clean variables used to receive serial data.
		inputString = "";
		stringComplete = false;
		memset(&inputArray[0], 0, sizeof(inputArray));
	}

	// LED STATUS ZONE AND CONTROL
	switch (currentStatus)
	{
	case 0:
		if (oldStatus != 0)
		{
			if (triggerUV == 2)
			{
				finishUS();
			}

			timer_ = millis();
			finishCS(LightsOff);

			if (timer_ - timer < delay)
			{
				Serial.println("E");
			}

			IsCSOff = false;
			IsNewRep = true;
			currentRep = 0;
			oldStatus = 0;
			triggerUV = 0;
			CSUS = 0;
			digitalWrite(PMTGatingPin, LOW);
		}
		break;

	case 1:
		LightsOff = false;
		if (oldStatus != 1)
		{

			if (IsNewRep)
			{
				IsNewRep = false;
				IsCSOff = false;

				if (CSUS != 0)
				{
					triggerUV = 1;
				}

				if (CSUS != 2)
				{
					digitalWrite(latchPin, LOW);

					timer = millis();
					CSOnset = millis();

					if (currentRep == 0)
					{
						Serial.println("B");
					}

					switch (LorR)
					{
					case 0: //* Left side
						shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
						shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
						shiftOut(dataPin, clockPin, LSBFIRST, 0b00001100);
						break;
					case 1: //* Right side
						shiftOut(dataPin, clockPin, LSBFIRST, 0b11000000);
						shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
						shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
						break;
						// case 2:
						//  shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
						//  shiftOut(dataPin, clockPin, LSBFIRST, 0b00001000);
						//  shiftOut(dataPin, clockPin, LSBFIRST, 0b10000000);
						//  break;
						// }
					}
					digitalWrite(latchPin, HIGH);
				}
			}

			if (CSUS != 2)
			{
				if (millis() - timer >= delay)
				{

					if (!IsCSOff)
					{
						finishCS(LightsOff);
						Serial.println("E");
						IsCSOff = true;
					}

					if (CSUS == 0 && (triggerUV == 0 || triggerUV == 3) || (CSUS == 1 && triggerUV == 3))
					{
						currentRep++;

						if (currentRep == numberReps)
						{
							currentStatus = 0;
							oldStatus = 1;
						}
						IsNewRep = true;
					}
				}
			}

			//* Switch on the UV LED
			if (triggerUV == 1 && (CSUS == 0 || (CSUS == 1 && millis() - CSOnset >= pulseLatency) || CSUS == 2))
			{
				timerUV = millis();
				OCR1A = pulsePower;
				Serial.println("b");
				triggerUV = 2; // triggerUV == 2 means that the UV LED is blinking.
			}

			//* Switch off the UV LED
			else if (triggerUV == 2 && millis() - timerUV >= USDuration)
			{
				finishUS();

				triggerUV = 3; // triggerUV == 3 means that the UV LED has already blinked.

				if (CSUS == 2)
				{
					currentStatus = 0;
					oldStatus = 1;
				}
			}
			break;
		}
	}
}

// Waits for serial data, and it's called everytime new data comes
// Full commands always end in '\n'
void serialEvent()
{
	while (Serial.available())
	{
		if (!stringComplete)
		{
			char inChar = (char)Serial.read();
			inputString += inChar;
			if (inChar == '\n')
				stringComplete = true;
		}
	}
}

int generateCycle(float input)
{
	return (int)(min(mainPWMCycle, mainPWMCycle - input * 10 + 1));
}

void finishCS(boolean realClean)
{
	analogWrite(outputEnablePin, 0); // OE (Output Enable) pin is always at 0 V, meaning that the ouput pins are always enabled.
	digitalWrite(latchPin, LOW);
	if (realClean)
	{
		//! Here could use the SRCLR (Shift Register Clear) pin, but this pin is connected to VCC.
		shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
		shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
		shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
	}
	else
	{
		shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
		shiftOut(dataPin, clockPin, LSBFIRST, 0b00001000); // front right
		shiftOut(dataPin, clockPin, LSBFIRST, 0b10000000); // front left
	}
	digitalWrite(latchPin, HIGH);
	analogWrite(outputEnablePin, 0);
}

void finishUS()
{
	OCR1A = mainPWMCycle;
	Serial.println("e");
}

// int powerLEDCS(float powerCS)
//{
//   return (int)((255 - powerCS) / 255);
// }
