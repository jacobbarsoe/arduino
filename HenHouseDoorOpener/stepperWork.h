/*
 * stepperWork.h
 *
 *  Created on: 30/09/2015
 *      Author: jabkj
 */

#ifndef STEPPERWORK_H_
#define STEPPERWORK_H_

#include <AccelStepper.h>

AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);

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

void setupSteppers()
{
	pinMode(STEPPER_POWER, OUTPUT);
	pinMode(STEPPER_DIR, OUTPUT);
	pinMode(STEPPER_STEP, OUTPUT);
	stepper.setMaxSpeed(100000);
	stepper.setAcceleration(1000);

	stepper.setCurrentPosition(CLOSE);

	disableStepperPower();
}

void togglePosition()
{
	long pos = stepper.currentPosition();
	//restore
	enableStepperPower();
	if (pos == CLOSE)
	{
		stepper.runToNewPosition(OPEN);
	}
	else
	{
		stepper.runToNewPosition(CLOSE);
	}
	disableStepperPower();
}

void runToOpenPosition()
{
	//restore
	enableStepperPower();
	stepper.runToNewPosition(OPEN);
	disableStepperPower();
}

void runToClosePosition()
{
	//restore
	enableStepperPower();
	stepper.runToNewPosition(CLOSE);
	disableStepperPower();
}

#endif /* STEPPERWORK_H_ */
