
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
#define RTC_VCC 6

#define JABK_SERIAL_DEBUG

#ifdef  JABK_SERIAL_DEBUG
#define TRACE(x) Serial.println(x)
#else
#define TRACE(x)
#endif

AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);
//TimeLord timelord;
//Hinnerup DK, 56.16N,10.4 Ø

int pos = 36000;

//print date to Serial
void printDate(time_t t)
{
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//print date and time to Serial
void printDateTime(time_t t)
{
    printDate(t);
    Serial << ' ';
    printTime(t);
}

//print time to Serial
void printTime(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}



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

void setRTCVCC(int on)
{
	digitalWrite(RTC_VCC, 0);
}

void syncClockToRTC()
{
	TRACE("syncClockToRTC 1");
    setRTCVCC(1);
	time_t t = RTC.get();
	TRACE("syncClockToRTC 2");
	setTime(t);
	TRACE("syncClockToRTC 3");
    //setRTCVCC(0);
}

void isr()
{
	if ( RTC.alarm(ALARM_1) ) {		//has Alarm1 triggered?
		TRACE("ALARM!!!");
	}
}

void setNextWakeUpFake()
{
	syncClockToRTC();
	adjustTime(20);
	RTC.setAlarm(ALM2_MATCH_DAY, minute(), hour(), day());
}

void setup()
{  
#ifdef JABK_SERIAL_DEBUG
	Serial.begin(115200);
	TRACE("Test");
#endif
	return;

	pinMode(STEPPER_POWER, OUTPUT);
	pinMode(STEPPER_DIR, OUTPUT);
	pinMode(STEPPER_STEP, OUTPUT);
	stepper.setMaxSpeed(100000);
	stepper.setAcceleration(1000);

	stepper.setCurrentPosition(0);

	digitalWrite(STEPPER_POWER, 0);
	digitalWrite(STEPPER_DIR, 1);
	digitalWrite(STEPPER_STEP, 1);

	//handle RTC time clock etc
    pinMode(RTC_VCC, OUTPUT);
    syncClockToRTC();
   // timelord.Position(56.16,-10.4);

    //RTC.alarmInterrupt(ALARM_1, true);
    TRACE("Before setNextWakeUpFake");
    setNextWakeUpFake();

	enable_sleepmodes();
	TRACE("After enable_sleepmodes");
//	attachInterrupt(0, isr, LOW);
	//pinMode(RTC_IRQ, INPUT);   //d2

}

float getTemperature()
{
    return RTC.temperature() / 4.;
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

void setRTCAlarmSunset(int minutesFromMidNight)
{
	uint8_t _today[tl_year+1];
	today(_today);
	byte hours = minutesFromMidNight % 60;
	byte minutes = minutesFromMidNight - hours*60;
	RTC.setAlarm(ALM1_MATCH_DAY, minutes, hours, _today[tl_day]);
}

void setRTCAlarmSunrise(int minutesFromMidNight)
{
	uint8_t _today[tl_year+1];
	today(_today);
	byte hours = minutesFromMidNight % 60;
	byte minutes = minutesFromMidNight - hours*60;
	RTC.setAlarm(ALM1_MATCH_DAY, minutes, hours, _today[tl_day]);
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

bool isBeforeSunrise()
{
	uint8_t sunrise[tl_year+1];
	today(sunrise);
	//timelord.SunRise(sunrise);
	return (hour()*60 + minute() > sunrise[tl_minute]);
}

bool isBeforeSunset()
{
	uint8_t sunset[tl_year+1];
	today(sunset);
	//timelord.SunSet(sunset);
	return (hour()*60 + minute() <	sunset[tl_minute]);
}

void setNextWakeUp()
{
	syncClockToRTC();

	if (!isBeforeSunrise() && isBeforeSunset())
	{
		uint8_t sunset[tl_year+1];
		today(sunset);
		//timelord.SunSet(sunset);
		setRTCAlarmSunset(sunset[tl_minute]);
	}
	else
	{
		long day = 60;
		day *= 60;
		day *= 24;
		adjustTime(day);

		uint8_t sunrise[tl_year+1];
		today(sunrise);
		//timelord.SunRise(sunrise);
		setRTCAlarmSunrise(sunrise[tl_minute]);

		syncClockToRTC();
	}
}


// The loop function is called in an endless loop
void loop()
{
	//system_sleep();
	TRACE("TEST2");
	if (RTC.alarm(ALARM_1))
	{
		//restore
		enableStepperPower();
		stepper.runToNewPosition(pos);
		pos = -pos;
		disableStepperPower();
	}
	TRACE("TEST3");
	
	  
}
