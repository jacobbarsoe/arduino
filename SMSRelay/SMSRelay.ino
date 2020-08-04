#include "Arduino.h"
#include <Arduino.h>
#define TINY_GSM_MODEM_M590

// Set serial for debug console (to the Serial Monitor, speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
//#define SerialAT Serial1

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(3, 2); // RX, TX


//#define DUMP_AT_COMMANDS
#define TINY_GSM_DEBUG SerialMon

// Set phone numbers, if you want to test SMS and Calls
#define SMS_TARGET  "+4551547418"
//#define CALL_TARGET "+380xxxxxxxxx"

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "YourAPN";
const char user[] = "";
const char pass[] = "";

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

void setup() {
	// Set console baud rate
	SerialMon.begin(115200);
	delay(10);
	pinMode(7, OUTPUT);
	digitalWrite(7, LOW);
	// Set your reset, enable, power pins here

	delay(3000);

	// Set GSM module baud rate
	TinyGsmAutoBaud(SerialAT, 38400, 38400);
}

void loop() {

	// Restart takes quite some time
	// To skip it, call init() instead of restart()
	DBG("Initializing modem...");
	//if (!modem.restart()) {
	//	delay(10000);
	//	return;
	//}

	String modemInfo = modem.getModemInfo();
	DBG("Modem:", modemInfo);


	DBG("Waiting for network...");
	if (!modem.waitForNetwork()) {
		delay(10000);
		return;
	}

	if (modem.isNetworkConnected()) {
		DBG("Network connected");
	}

	bool res = modem.isGprsConnected();
	DBG("GPRS status:", res ? "connected" : "not connected");

	String ccid = modem.getSimCCID();
	DBG("CCID:", ccid);

	String imei = modem.getIMEI();
	DBG("IMEI:", imei);

	String cop = modem.getOperator();
	DBG("Operator:", cop);

	int csq = modem.getSignalQuality();
	DBG("Signal quality:", csq);



#if defined(SMS_TARGET)
	res = modem.sendSMS(SMS_TARGET, String("Hi, I rebooted"));
	DBG("SMS:", res ? "OK" : "fail");
#endif


	// Try to power-off (modem may decide to restart automatically)
	// To turn off modem completely, please use Reset/Enable pins
	//modem.poweroff();
	//DBG("Poweroff.");
int count = 0;
	// Do nothing forevermore
	while (true) {
		if (++count % 100 == 0 && modem.receiveSMS("OFF", false))
		{
			DBG("Relay OFF");
			digitalWrite(7, HIGH);
		}
		if (count % 100 == 0 && modem.receiveSMS("ON"))
		{
			DBG("Relay ON");
			digitalWrite(7, LOW);
		}
		modem.maintain();
	}
}
