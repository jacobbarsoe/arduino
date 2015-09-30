/*
 * stepperWork.h
 *
 *  Created on: 30/09/2015
 *      Author: jabkj
 */

#ifndef STEPPERWORK_H_
#define STEPPERWORK_H_

#include <AccelStepper.h>

#define STEPPER_DIR 4
#define STEPPER_STEP 5
#define STEPPER_POWER 8

AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);

void setupSteppers()
{
	pinMode(STEPPER_POWER, OUTPUT);
	pinMode(STEPPER_DIR, OUTPUT);
	pinMode(STEPPER_STEP, OUTPUT);
	stepper.setMaxSpeed(100000);
	stepper.setAcceleration(1000);

	stepper.setCurrentPosition(OPEN * length);

	digitalWrite(STEPPER_POWER, 0);
	digitalWrite(STEPPER_DIR, 1);
	digitalWrite(STEPPER_STEP, 1);
}


void enableStepperPower()
{
	digitalWrite(STEPPER_POWER, 1);
}

void disableStepperPower()
{
	digitalWrite(STEPPER_POWER, 0);
	digitalWrite(STEPPER_DIR, 1);
	digitalWrite(STEPPER_STEP, 1);

}

void togglePosition()
{
	long pos = stepper.currentPosition();
	//restore
	enableStepperPower();
	if (pos == CLOSE*length)
	{
		stepper.runToNewPosition(OPEN*length);
	}
	else
	{
		stepper.runToNewPosition(CLOSE*length);
	}
	disableStepperPower();
}

void runToOpenPosition()
{
	//restore
	enableStepperPower();
	stepper.runToNewPosition(OPEN * length);
	disableStepperPower();
}

void runToClosePosition()
{
	//restore
	enableStepperPower();
	stepper.runToNewPosition(CLOSE * length);
	disableStepperPower();
}

#endif /* STEPPERWORK_H_ */
