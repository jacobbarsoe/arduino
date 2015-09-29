
#include <AccelStepper.h>

#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <MySleep.h>

#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Streaming.h>
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire

#include <TimeLord.h>

#define STEPPER_DIR 4
#define STEPPER_STEP 5
#define STEPPER_POWER 8
#define RTC_IRQ 2
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

AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);
TimeLord timelord;
//Hinnerup DK, 56.16N,10.4 Ø

#define step_mm 4000 //0.9 deg/step , 1/8 stepping. gevind 0.8mm => 360/0.9*8/0.8
#define step_cm (long)10*step_mm
long length = (long)200*step_mm;

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}

//print time to Serial
void printTime(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}

//print date to Serial
void printDate(time_t t)
{
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//print date and time to Serial
void printDateTime(time_t t)
{
#ifdef JABK_SERIAL_DEBUG
    printDate(t);
    Serial << ' ';
    printTime(t);
    Serial << endl;
#endif
}


void setRTCVCC(int on)
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

void isr()
{
}

volatile bool buttonPress = false;
void btnisr()
{
	delay(50);
	//buttonPress= true;
}

void setNextWakeUp();

void setup()
{  
#ifdef JABK_SERIAL_DEBUG
	Serial.begin(115200);
	TRACE("Starting");
#endif

	pinMode(STEPPER_POWER, OUTPUT);
	pinMode(STEPPER_DIR, OUTPUT);
	pinMode(STEPPER_STEP, OUTPUT);
	stepper.setMaxSpeed(100000);
	stepper.setAcceleration(1000);

	stepper.setCurrentPosition(OPEN * length);

	digitalWrite(STEPPER_POWER, 0);
	digitalWrite(STEPPER_DIR, 1);
	digitalWrite(STEPPER_STEP, 1);

	//handle RTC time clock etc
    pinMode(RTC_VCC, OUTPUT);
    timelord.Position(56.16,-10.4);
    syncClockToRTC();
	setNextWakeUp();

	enable_sleepmodes();

	pinMode(RTC_IRQ, INPUT);   //d2
	pinMode(BTN_IRQ, INPUT);   //d3

	attachInterrupt(0, isr, FALLING);
	attachInterrupt(1, btnisr, RISING);

	setup_watchdog(9);
}

float getTemperature()
{
	float temp;
	setRTCVCC(1);
    temp = RTC.temperature() / 4.;
	setRTCVCC(0);
    return temp;
}

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

void runToOpenPosition()
{
	//restore
	enableStepperPower();
	stepper.runToNewPosition(OPEN * length);
	disableStepperPower();
}

void runToClosePosition()
{
	//restore
	enableStepperPower();
	stepper.runToNewPosition(CLOSE * length);
	disableStepperPower();
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

typedef struct day_t
{
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
} Today;

void setAlarm(int minutesFromMidNight, Today *day)
{
	uint8_t _today[tl_year+1];
	today(_today);
	byte hours = minutesFromMidNight % 60;
	byte minutes = minutesFromMidNight - hours*60;
	_today[tl_minute] = minutes;
	_today[tl_hour] = hours;

	memcpy(_today, day, sizeof(Today));
}


bool isBeforeSunrise()
{
	uint8_t sunrise[tl_year+1];
	today(sunrise);
	timelord.SunRise(sunrise);
	return (hour()*60 + minute() > sunrise[tl_minute]);
}

bool isBeforeSunset()
{
	uint8_t sunset[tl_year+1];
	today(sunset);
	timelord.SunSet(sunset);
	return (hour()*60 + minute() <	sunset[tl_minute]);
}


Today nextSunrise;
Today nextSunset;
void setNextWakeUp()
{
	printDateTime(now());
	if (!isBeforeSunrise() && isBeforeSunset())
	{
		TRACE("Setting next event on Sunset later today");
		uint8_t next_sunset[tl_year+1];
		today(next_sunset);
		timelord.SunSet(next_sunset);
		setAlarm(next_sunset[tl_minute], &nextSunset );
	}
	else if (isBeforeSunrise() && isBeforeSunset())
	{
		TRACE("Setting next event on Sunrise later today");
		uint8_t next_sunrise[tl_year+1];
		today(next_sunrise);
		timelord.SunRise(next_sunrise);
		setAlarm(next_sunrise[tl_minute], &nextSunrise);
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
		setAlarm(next_sunrise[tl_minute], &nextSunrise);

		syncClockToRTC();
	}
}

volatile int f_wdt = 0;
ISR(WDT_vect) {
  f_wdt++;
}

//check every minute if we are within the minute where we close or open
bool isTime(Today* next)
{
	return (next->year == year() &&
			next->month == month() &&
			next->day == day() &&
			next->hour*60+next->minute == hour()*60+minute());
}

void togglePosition()
{
	long pos = stepper.currentPosition();
	//restore
	enableStepperPower();
	if (pos == CLOSE*length)
	{
		stepper.runToNewPosition(OPEN*length);
	}
	else
	{
		stepper.runToNewPosition(CLOSE*length);
	}
	disableStepperPower();
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
