#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Time.h>
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire

void setup_watchdog(int ii) ;
void system_sleep() ;
ISR(WDT_vect) ;
void enable_sleepmodes() ;
void setup() ;
void loop() ;

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
  byte bb;

  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);

  MCUSR &= ~(1<<WDRF);

  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}

//****************************************************************
// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

int f_wdt = 0;
//****************************************************************
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
  ++f_wdt;  // set global flag  sei();
}

void enable_sleepmodes()
{
	  // CPU Sleep Modes
	  // SM2 SM1 SM0 Sleep Mode
	  // 0    0  0 Idle
	  // 0    0  1 ADC Noise Reduction
	  // 0    1  0 Power-down
	  // 0    1  1 Power-save
	  // 1    0  0 Reserved
	  // 1    0  1 Reserved
	  // 1    1  0 Standby(1)
	  cbi( SMCR,SE );      // sleep enable, power down mode
	  cbi( SMCR,SM0 );     // power down mode
	  sbi( SMCR,SM1 );     // power down mode
	  cbi( SMCR,SM2 );     // power down mode
}

void setup()
{
	setup_watchdog(8); // 4sec
	enable_sleepmodes();
	pinMode(13, OUTPUT);
}

// The loop function is called in an endless loop
void loop()
{
	system_sleep();
	// turn off I2C
	digitalWrite(SDA,0);
	digitalWrite(SCL,0);
	TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));
	if (f_wdt>0)
	{
		//restore
		digitalWrite(13,1);
		delay(1000);
		digitalWrite(13,0);
		f_wdt=0;
	}
}
