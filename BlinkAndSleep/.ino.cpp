//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2015-10-15 21:40:49

#include "Arduino.h"
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>
void setup_watchdog(int ii) ;
void system_sleep() ;
ISR(WDT_vect) ;
void enable_sleepmodes() ;
void setup() ;
void loop() ;

#include "BlinkAndSleep.ino"

