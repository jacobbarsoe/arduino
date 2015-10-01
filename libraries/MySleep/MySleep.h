
#ifndef ARDUINO_MYSLEEP
#define ARDUINO_MYSLEEP

#include <avr/sleep.h>

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

  //disable brownout
  MCUCR = bit (BODS) | bit (BODSE);  // turn on brown-out enable select
  MCUCR = bit (BODS);        // this must be done within 4 clock cycles of above
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

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep()
{
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

#endif // ARDUINO_MYSLEEP
