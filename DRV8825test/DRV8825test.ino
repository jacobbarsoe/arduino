/*
 * Microstepping demo
 *
 * This requires that M0, M1 be connected in addition to STEP,DIR
 *
 * Copyright (C)2015 Laurentiu Badea
 *
 * This file may be redistributed under the terms of the MIT license.
 * A copy of this license has been included with this distribution in the file LICENSE.
 */
#include <Arduino.h>
#include "DRV8834.h"
#include "A4988.h"
#include "DRV8825.h"

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200

// All the wires needed for full functionality
#define DIR 28
#define STEP 26

// 2-wire basic config, microstepping is hardwired on the driver
// BasicStepperDriver stepper(DIR, STEP);

DRV8825 stepper(MOTOR_STEPS, DIR, STEP);

void setup() {
    /*
     * Set target motor RPM.
     * These motors can do up to about 200rpm.
     * Too high will result in a high pitched whine and the motor does not move.
     */
    stepper.setRPM(60);
    pinMode(24    , OUTPUT);
}

void loop() {
  //  delay(1000);

    stepper.setMicrostep(32);

    // 180 degrees now takes 100 * 8 microsteps
    stepper.move(100*8);
    stepper.rotate(180);

    // as you can see, using degrees is easier
    stepper.move(-100*8);
    stepper.rotate(-180);

  //  delay(5000);
}
