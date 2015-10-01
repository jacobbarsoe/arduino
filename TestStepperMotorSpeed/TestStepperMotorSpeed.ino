#include <Arduino.h>


#define DIR_PIN 4
#define STEP_PIN 5

void setup() {
  pinMode(8, OUTPUT);
  digitalWrite(8, 1);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
}

void loop(){

  //rotate a specific number of degrees
  rotateDeg(36000, 1);
  delay(1000);

  rotateDeg(-36000, 1);  //reverse
  delay(1000);


  //rotate a specific number of microsteps (8 microsteps per step)
  //a 200 step stepper would take 1600 micro steps for one full revolution
  rotate(1600, .5);
  delay(1000);

  rotate(-1600, .25); //reverse
  delay(1000);
}



void rotate(int steps, float speed){
  //rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
  //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
  int dir = (steps > 0)? HIGH:LOW;
  steps = abs(steps);

  digitalWrite(DIR_PIN,dir);

  float usDelay = (1/speed) * 35;

  for(int i=0; i < steps; i++){
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(usDelay);

    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(usDelay);
  }
}

void rotateDeg(float deg, float speed){
  //rotate a specific number of degrees (negitive for reverse movement)
  //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
  int dir = (deg > 0)? HIGH:LOW;
  digitalWrite(DIR_PIN,dir);

  int steps = abs(deg)*(1/0.225);
  float usDelay = (1/speed) * 70;

  for(int i=0; i < steps; i++){
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(usDelay);

    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(usDelay);
  }
}
