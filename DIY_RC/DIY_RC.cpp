
 /*
  * DIY RC car - JABK - based on ReadReceiver example
  *
 Copyright 2011 Lex Talionis (Lex.V.Talionis at gmail)
 This program is free software: you can redistribute it
 and/or modify it under the terms of the GNU General Public
 License as published by the Free Software Foundation,
 either version 3 of the License, or (at your option) any
 later version.

 This uses pin change interrupts and timer 1 to mesure the
 time between the rise and fall of 3 channels of PPM
 (Though often called PWM, see http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1253149521/all)
 on a typical RC car reciver.  It could be extended to as
 many channels as you like.
 */
// Do not remove the include below
#include "DIY_RC.h"

#include <PinChangeInt.h>	// http://www.arduino.cc/playground/Main/PinChangeInt
#include <TimerOne.h>		// http://www.arduino.cc/playground/Code/Timer1

// Assign your channel in pins
#define THROTTLE_IN_PIN 5
#define STEERING_IN_PIN 4
#define AUX_IN_PIN 4

//L298N inputs
#define IN1 11
#define IN2 12
#define IN3 13
#define IN4 14

#define ENABLE_A 10
#define ENABLE_B 15

#define NO_PORTB_PINCHANGES     //PinChangeInt setup
#define NO_PORTC_PINCHANGES	//only port D pinchanges (see: http://www.arduino.cc/playground/Learning/Pins)
#define PIN_COUNT 2	//number of channels attached to the reciver
#define MAX_PIN_CHANGE_PINS PIN_COUNT

#define RC_TURN STEERING_IN_PIN	//arduino pins attached to the reciver
#define RC_FWD THROTTLE_IN_PIN
byte pin[] = {RC_FWD, RC_TURN, AUX_IN_PIN};	//for maximum efficency thise pins should be attached
volatile unsigned int time[] = {0,0,0};				// to the reciver's channels in the order listed here

byte state=0;
byte burp=0;    // a counter to see how many times the int has executed
byte cmd=0;     // a place to put our serial data
byte i=0;       // global counter for tracking what pin we are on

void rise()		//on the rising edge of the currently intresting pin
{
	Timer1.restart();		//set our stopwatch to 0
	Timer1.start();			//and start it up
	state=RISING;
	burp++;
}

void fall()		//on the falling edge of the signal
{
	state=FALLING;

	time[i]=Timer1.read();	// Needs Timer1-v2
	Timer1.stop();
}

//The setup function is called once at startup of the sketch
void setup()
{
	Serial.begin(115200);
	pinMode(IN1, OUTPUT);
	pinMode(IN2, OUTPUT);
	digitalWrite(IN1, 0);
	digitalWrite(IN2, 0);
	pinMode(ENABLE_A, OUTPUT);
	pinMode(IN3, OUTPUT);
	pinMode(IN4, OUTPUT);
	digitalWrite(IN3, 0);
	digitalWrite(IN4, 0);
	pinMode(ENABLE_B, OUTPUT);

	digitalWrite(ENABLE_A, 1);
	digitalWrite(ENABLE_B, 1);
	Timer1.initialize(2200);	//longest pulse in PPM is usally 2.1 milliseconds,
							    //pick a period that gives you a little headroom.
	Timer1.stop();				//stop the counter
	Timer1.restart();			//set the clock to zero

	for (int j=0; j<PIN_COUNT; j++)
	{
		pinMode(pin[j], INPUT);     //set the pin to input
		digitalWrite(pin[j], HIGH); //use the internal pullup resistor
	}
	PCintPort::attachInterrupt(pin[0], rise, RISING); // attach a PinChange Interrupt to our first pin
}

 void computeRCTime()
 {
 	switch (state)
 	{
 		case RISING: //we have just seen a rising edge
 			PCintPort::detachInterrupt(pin[i]);
 			PCintPort::attachInterrupt(pin[i], fall, FALLING); //attach the falling end
 			state=254;
 			break;
 		case FALLING: //we just saw a falling edge
 			PCintPort::detachInterrupt(pin[i]);
 			i++;				//move to the next pin
 			i = i % PIN_COUNT;  //i ranges from 0 to PIN_COUNT
 			PCintPort::attachInterrupt(pin[i], rise,RISING);
 			state=255;
 			break;
 		/*default:
 			//do nothing
 			break;*/
 	}
 }
bool signalValid()
{
	return state == 255 &&
			i == 0 &&
			(time[0] < 2200 && time[0] > 800) &&
			(time[1] < 2200 && time[1] > 800);

}
 void printSerial()
 {
	 if (signalValid())
	 {
		 Serial.print("time:\t");
		 for (byte i=0; i<PIN_COUNT;i++)
		 {
			 Serial.print(i,DEC);
			 Serial.print(":");
			 Serial.print(time[i]-1500,DEC);
			 Serial.print("\t");
		 }
		 Serial.print(burp, DEC);
		 Serial.println();
	 }
 }

 void loop()
 {
	computeRCTime();
	printSerial();
#define MID_POINT 1500
#define OFFSET 25
	int fwd = (int)time[0]-MID_POINT;
	int steer = (int)time[1]-MID_POINT;

	if (signalValid())
	{
		if (fwd > OFFSET)
		{
			digitalWrite(IN2, 0);
			digitalWrite(IN1, 1);
			digitalWrite(ENABLE_A, 1);
		}
		else if (fwd < -OFFSET)
		{
			digitalWrite(IN2, 1);
			digitalWrite(IN1, 0);
			digitalWrite(ENABLE_A, 1);
		}
		else
		{
			digitalWrite(IN2, 0);
			digitalWrite(IN1, 0);
			digitalWrite(ENABLE_A, 0);
		}
		//analogWrite(ENABLE_A, min(512,abs(fwd))/2-1);

		if (steer > OFFSET)
		{
			digitalWrite(IN3, 0);
			digitalWrite(IN4, 1);
			digitalWrite(ENABLE_B, 1);
		}
		else if (steer < -OFFSET)
		{
			digitalWrite(IN3, 1);
			digitalWrite(IN4, 0);
			digitalWrite(ENABLE_B, 1);
		}
		else
		{
			digitalWrite(IN3, 0);
			digitalWrite(IN4, 0);
			digitalWrite(ENABLE_B, 0);
		}
		//analogWrite(ENABLE_B, min(512,abs(steer))/2-1);
	}


 }
