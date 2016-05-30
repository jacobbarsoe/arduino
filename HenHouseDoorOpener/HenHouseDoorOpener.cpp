#include <Arduino.h>

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>

#include <MySleep.h>

#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Streaming.h>
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire

#include <TimeLord.h>

#define BTN_IRQ 3
#define BTN_VCC 7
#define BTN_GND 15
#define RTC_VCC 6
#define RF_IO_GND_PIN 14
#define RF_IO_PWR_PIN 2 //9 and 10 is ce and csn
#define CHARGE_ENABLE 16

#define RF_CE 9
#define RF_CSN 10
#define STEPPER_DIR 4
#define STEPPER_STEP 5
#define STEPPER_POWER 8

//#define JABK_SERIAL_DEBUG

#ifdef  JABK_SERIAL_DEBUG
#define TRACE(x) Serial.println(x)
#else
#define TRACE(x)
#endif

#define step_mm 400 //0.9 deg/step , 1/8 stepping. gevind 8mm => 360/0.9*8/8
#define step_cm (long)10*step_mm

//door open length
//flip if direction is opposite
#define OPEN (long)230*step_mm //30cm
#define CLOSE (long)0

#include "PrintDataUtil.h"
#include "stepperWork.h"

//typedefs
typedef struct day_t
{
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
} Today;

//Globals
Today nextSunrise;
Today nextSunset;

#include "Radio.h"

TimeLord timelord; //Hinnerup DK, 56.16N,10.4

volatile bool buttonPress = false;

volatile int f_wdt = 0;


//ISRs
ISR(WDT_vect) {
  ++f_wdt;
}

void btnisr()
{
  delay(100);
  buttonPress= true;
}

void setRTCVCC(int on)
{
  digitalWrite(RTC_VCC, on);
  digitalWrite(SDA,on);
  digitalWrite(SCL,on);

  if (on)
	  TWCR |= (bit(TWEN) | bit(TWIE) | bit(TWEA));
  else
	  TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));

  delay(1);
}

time_t syncClockToRTC()
{
  time_t t;
  setRTCVCC(1);
  t = RTC.get();
  setTime(t);
  setRTCVCC(0);
  return t;
}

int getTemperature()
{
  int temp;
  setRTCVCC(1);
  temp = (RTC.temperature() +2 )/ 4;
  setRTCVCC(0);
  return temp;
}

void today(uint8_t* _today)
{
  _today[tl_second] = 0;
  _today[tl_minute] = 0;
  _today[tl_hour] = 12;
  _today[tl_day] = day();
  _today[tl_month] = month();
  _today[tl_year] = year()-2000;
}

void setAlarm(uint8_t hours, uint8_t minutes, Today *day)
{
  uint8_t _today[tl_year+1];
  today(_today);
  _today[tl_minute] = minutes;
  _today[tl_hour] = hours;

  memcpy(day, _today, sizeof(Today));

  TRACE("Alarm set to ");
  TRACE(day->year+2000);
  TRACE(day->month);
  TRACE(day->day);
  TRACE(day->hour);
  TRACE(day->minute);
}


bool isBeforeSunrise()
{
  uint8_t sunrise[tl_year+1];
  today(sunrise);
  timelord.SunRise(sunrise);
  return (hour()*60 + minute() < (int)sunrise[tl_hour]*60 + sunrise[tl_minute]);
}

bool isBeforeSunset()
{
  uint8_t sunset[tl_year+1];
  today(sunset);
  timelord.SunSet(sunset);
  return (hour()*60 + minute() < (int)sunset[tl_hour]*60 + sunset[tl_minute]);
}

void setNextWakeUp()
{
  printDateTime(now());
  if (!isBeforeSunrise() && isBeforeSunset())
  {
	  TRACE("Setting next event on Sunset later today");
	  uint8_t next_sunset[tl_year+1];

	  today(next_sunset);
	  timelord.SunSet(next_sunset);
	  //set time+40min on sunset to allow for the hens to get inside
	  uint8_t hour = next_sunset[tl_hour]+1;
	  uint8_t minute = next_sunset[tl_minute];

	  minute += 40;
	  if (minute >= 60)
	  {
		  minute -= 60;
		  ++hour;
	  }

	  setAlarm(hour, minute, &nextSunset );
  }
  else if (isBeforeSunrise() && isBeforeSunset())
  {
	  TRACE("Setting next event on Sunrise later today");
	  uint8_t next_sunrise[tl_year+1];

	  today(next_sunrise);
	  timelord.SunRise(next_sunrise);

	  //no later than 7 o'clock (6 gmt)
	  if (next_sunrise[tl_hour] > 5)
	  {
		  next_sunrise[tl_hour] = 5;
		  next_sunrise[tl_minute] = 30;
	  }
	  setAlarm(next_sunrise[tl_hour], next_sunrise[tl_minute], &nextSunrise);
  }
  else
  {
	  TRACE("Setting next event on Sunrise tomorrow");
      uint8_t next_sunrise[tl_year+1];

      //fake tomorrow
      adjustTime(((long)60)*60*24);
      today(next_sunrise);
      timelord.SunRise(next_sunrise);

	  //no later than 7 o'clock  (6 gmt)
	  if (next_sunrise[tl_hour] > 6)
	  {
		  next_sunrise[tl_hour] = 6;
		  next_sunrise[tl_minute] = 0;
	  }
      setAlarm(next_sunrise[tl_hour], next_sunrise[tl_minute], &nextSunrise);

      //resync time
      syncClockToRTC();
    }
}

bool isOpen()
{
	long pos = stepper.currentPosition();
	return pos == CLOSE ? false : true;
}


//check every minute if we are within the minute where we close or open
bool isTime(Today* next)
{
  TRACE("Next");
  TRACE(next->year);
  TRACE(next->month);
  TRACE(next->day);
  TRACE(next->hour);
  TRACE(next->minute);
  TRACE("Actual");
  TRACE(year());
  TRACE(month());
  TRACE(day());
  TRACE(hour());
  TRACE(minute());

  int minutesSinceMidnight = (int)next->hour * 60 + next->minute;
  int minutesSinceMidnightNow = hour() * 60 + minute();

  return (next->year == year()-2000 &&
	  next->month == month() &&
	  next->day == day() &&
	  (minutesSinceMidnightNow > minutesSinceMidnight &&
	   minutesSinceMidnightNow < minutesSinceMidnight + 2));
}

void trickleCharge()
{
	const int TRICKLE_CHARGE_START = 4100;
	const int TRICKLE_CHARGE_STOP = 4200;

	int vcc = readVcc();
	if (vcc < TRICKLE_CHARGE_START)
	{
		digitalWrite(CHARGE_ENABLE, LOW);
	}
	else if (vcc > TRICKLE_CHARGE_STOP)
	{
		digitalWrite(CHARGE_ENABLE, HIGH);
	}
	else
	{
		//hysterisis
	}
}

void setup()
{
#ifdef JABK_SERIAL_DEBUG
  Serial.begin(115200);
  TRACE("Starting");
#endif

  setupSteppers();

  //setup cpu sleep via watchdog
  setup_watchdog(9);
  enable_sleepmodes();

  //btn IRQ
  pinMode(BTN_IRQ, INPUT);   //d3
  pinMode(BTN_VCC, OUTPUT);
  digitalWrite(BTN_VCC,1);

  pinMode(RF_IO_PWR_PIN, OUTPUT);
#ifdef RF_IO_GND_PIN
  pinMode(RF_IO_GND_PIN, OUTPUT);
  digitalWrite(RF_IO_GND_PIN, LOW);
#endif
#ifdef BTN_GND
  pinMode(BTN_GND, OUTPUT);
  digitalWrite(BTN_GND, LOW);
#endif
#ifdef CHARGE_ENABLE
  pinMode(CHARGE_ENABLE, OUTPUT);
  digitalWrite(CHARGE_ENABLE, LOW);
#endif

  pinMode(RTC_VCC, OUTPUT);
  setRTCVCC(0);

  //on startup - sync time with GW (rPI)
  sendOverRadio(true);

  //handle RTC time clock etc
  timelord.TimeZone(0); //time sync from gw is in GMT. do not bother localtime
  timelord.Position(56.16,10.4); //west and south negative
  syncClockToRTC();
  setNextWakeUp();
}

// The loop function is called in an endless loop
void loop()
{
  static int hourCounter = 0;
#ifdef JABK_SERIAL_DEBUG
  delay(100);
#endif

//  attachInterrupt(1, btnisr, LOW);//only level irq in deep power down
  system_sleep();
 // detachInterrupt(1);

#ifdef JABK_SERIAL_DEBUG
  if (f_wdt > 0) //wake every 9 sec
#else
  if (f_wdt > 6) //wake every 54 sec
#endif
  {
      syncClockToRTC();
      f_wdt = 0; //reset timer
      //checks if we are at the minute where this happens
      if (isTime(&nextSunset) && isOpen())
      {
    	  TRACE("Closing Start");
		  printDateTime(now());
		  runToClosePosition();
		  printDateTime(now());
		  TRACE("Closing End");
		  setNextWakeUp();
		  sendOverRadio();
      }
      else if (isTime(&nextSunrise) && !isOpen())
      {
		  TRACE("Opening Start");
		  printDateTime(now());
		  runToOpenPosition();
		  printDateTime(now());
		  TRACE("Opening End");
		  setNextWakeUp();
		  sendOverRadio();
      }
  }
  if (buttonPress)
  {
	  TRACE("Toggle Position");
      togglePosition();
      buttonPress = false;
      TRACE("Toggle Position DONE");
	  sendOverRadio();
  }
  if (++hourCounter > 38) //1800*1.15/54 (15% drift due to wdog on bat voltage approx 3V)
  {
	  hourCounter = 0;
	  setNextWakeUp(); //compute every 5min
	  sendOverRadio();
	  trickleCharge();
  }
}
