
#include <AccelStepper.h>

AccelStepper stepper(AccelStepper::FULL4WIRE, 10, 12, 11, 13);
int endPoint  =10000;
//The setup function is called once at startup of the sketch
void setup()
{
	stepper.setMaxSpeed(1000.0);
	stepper.setAcceleration(100.0);
	stepper.setSpeed(200);
	stepper.setCurrentPosition(0);
	stepper.moveTo(endPoint);
}

// The loop function is called in an endless loop
void loop()
{
  //Change direction at the limits
	if (stepper.distanceToGo() == 0)
   {
	 stepper.setCurrentPosition(0);
	 endPoint = -endPoint;
	 stepper.moveTo(endPoint);
   }
	stepper.run();
}
