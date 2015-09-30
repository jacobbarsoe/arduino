#include <Arduino.h>

#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <MySleep.h>

#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Streaming.h>
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire

#include <TimeLord.h>

#define BTN_IRQ 3
#define RTC_VCC 6

//flip if direction is opposite
#define OPEN 1
#define CLOSE 0

#define JABK_SERIAL_DEBUG

#ifdef  JABK_SERIAL_DEBUG
#define TRACE(x) Serial.println(x)
#else
#define TRACE(x)
#endif

#define step_mm 4000 //0.9 deg/step , 1/8 stepping. gevind 0.8mm => 360/0.9*8/0.8
#define step_cm (long)10*step_mm


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

TimeLord timelord; //Hinnerup DK, 56.16N,10.4

//door open length
long length = (long)200*step_mm; //20cm

volatile bool buttonPress = false;

volatile int f_wdt = 0;


//ISRs
ISR(WDT_vect) {
	f_wdt++;
}

void btnisr()
{
	delay(50);
	//buttonPress= true;
}

inline void setRTCVCC(int on)
{
	digitalWrite(RTC_VCC, on);
}

void syncClockToRTC()
{
	setRTCVCC(1);
	time_t t = RTC.get();
	setTime(t);
	setRTCVCC(0);
}

float getTemperature()
{
	float temp;
	setRTCVCC(1);
	temp = RTC.temperature() / 4.;
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
	_today[tl_year] = year();
}

void setAlarm(uint8_t hours, uint8_t minutes, Today *day)
{
	uint8_t _today[tl_year+1];
	today(_today);
	_today[tl_minute] = minutes;
	_today[tl_hour] = hours;

	memcpy(_today, day, sizeof(Today));
}


bool isBeforeSunrise()
{
	uint8_t sunrise[tl_year+1];
	today(sunrise);
	timelord.SunRise(sunrise);
	return (hour()*60 + minute() > sunrise[tl_hour]*60 + sunrise[tl_minute]);
}

bool isBeforeSunset()
{
	uint8_t sunset[tl_year+1];
	today(sunset);
	timelord.SunSet(sunset);
	return (hour()*60 + minute() <	sunset[tl_hour]*60 + sunset[tl_minute]);
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
		setAlarm(next_sunset[tl_hour], next_sunset[tl_minute], &nextSunset );
	}
	else if (isBeforeSunrise() && isBeforeSunset())
	{
		TRACE("Setting next event on Sunrise later today");
		uint8_t next_sunrise[tl_year+1];
		today(next_sunrise);
		timelord.SunRise(next_sunrise);
		setAlarm(next_sunrise[tl_hour], next_sunrise[tl_minute], &nextSunrise);
	}
	else
	{
		TRACE("Setting next event on Sunrise tomorrow");
		long day = 60;
		day *= 60;
		day *= 24;
		adjustTime(day);

		uint8_t next_sunrise[tl_year+1];
		today(next_sunrise);
		timelord.SunRise(next_sunrise);
		setAlarm(next_sunrise[tl_hour], next_sunrise[tl_minute], &nextSunrise);

		syncClockToRTC();
	}
}

//check every minute if we are within the minute where we close or open
bool isTime(Today* next)
{
	return (next->year == year() &&
			next->month == month() &&
			next->day == day() &&
			next->hour*60+next->minute == hour()*60+minute());
}


void setup()
{
#ifdef JABK_SERIAL_DEBUG
	Serial.begin(115200);
	TRACE("Starting");
#endif

	setupSteppers();

	//handle RTC time clock etc
	pinMode(RTC_VCC, OUTPUT);
	timelord.Position(56.16,-10.4);
	syncClockToRTC();
	setNextWakeUp();

	//setup cpu sleep via watchdog
	setup_watchdog(9);
	enable_sleepmodes();

	//btn IRQ
	pinMode(BTN_IRQ, INPUT);   //d3
	attachInterrupt(1, btnisr, RISING);
}

// The loop function is called in an endless loop
void loop()
{
	system_sleep();
	if (f_wdt > 6) //wake every 54 sec
	{
		syncClockToRTC();
		f_wdt = 0; //reset timer
		//checks if we are at the minute where this happens
		if (isTime(&nextSunset))
		{
			printDateTime(now());
			TRACE("Closing");
			runToClosePosition();

			setNextWakeUp();
		}
		else if (isTime(&nextSunrise))
		{
			printDateTime(now());
			TRACE("Opening");
			runToOpenPosition();

			setNextWakeUp();
		}
	}
	else if (buttonPress)
	{
		togglePosition();
		buttonPress = false;
	}
}
