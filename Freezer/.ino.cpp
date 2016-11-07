//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2016-11-07 23:08:56

#include "Arduino.h"
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <MySleep.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "NTC.h"
#include "SmoothingFilter.h"
ISR(WDT_vect) ;
void setup_radio() ;
void sendOverRadio() ;
void RF24_system_sleep() ;
void setup() ;
void sampleTemp() ;
void turnCompressorOn();
void turnCompressorOff();
boolean isCompressorOff();
boolean isCompressorOn();
void freezerControl() ;
void loop() ;

#include "Freezer.ino"

