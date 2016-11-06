/*
  Send freezer data over RF24 and control temp

 go to deep sleep in between transmits (one way)
 */
#include <Arduino.h>

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <MySleep.h>

// NRF24L01
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#include "NTC.h"

//#define JABK_DEBUG

#ifdef JABK_DEBUG
#define TRACE(x) Serial.println(x)
#define TRACE_GENERIC(x) x
#else
#define TRACE(x)
#define TRACE_GENERIC(x)
#endif

//pins
#define RF_IO_PWR_PIN       8
#define SAMPLE_NTC_ADC     3
#define relayPin           7

//Globals
RF24 radio(9,10); // Set up nRF24L01 radio on SPI pin for CE, CSN
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0x7365727631LL };
uint16_t nodeID = 2; //1=el, 2=freezer
char receivePayload[32];

volatile int WakeUpTime;
volatile float temp = 0.0f;
uint8_t counter = 0;
volatile int flag = 0;
int state = 0;
volatile bool debounce = false;
volatile int sleep_mode = SLEEP_MODE_PWR_DOWN;

NTC ntc(SAMPLE_NTC_ADC, 100000.0f, 303000.0f, 25.0f, 3950);
//****************************************************************
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect)
{
	++WakeUpTime;
}

void setup_radio()
{
  //CONFIGURE RADIO
  radio.begin();
  // Enable this seems to work better
  radio.enableDynamicPayloads();
  radio.setAutoAck(1);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(70);
  radio.setRetries(15,15);
  radio.setCRCLength(RF24_CRC_8);

  radio.openWritingPipe(pipes[0]);
  //radio.openReadingPipe(1,pipes[1]);
}

void sendOverRadio()
{
  char outBuffer[16];
  memset(outBuffer, 0, 16);
  digitalWrite(RF_IO_PWR_PIN, HIGH);
  delay(1);
  setup_radio();
  TRACE("setup");
  radio.powerUp();
  TRACE("powerup");

  // Append the hex nodeID to the beginning of the payload
  char str_temp[6];
  dtostrf(temp, 4, 2, str_temp);
  sprintf(outBuffer,"%2X,%03d,%s,%01d",nodeID,++counter,str_temp,state);

  // Stop listening and write to radio
  radio.stopListening();
  TRACE(outBuffer);
  TRACE("stoplistening");

  // Send to hub
  if ( radio.write( outBuffer, strlen(outBuffer)) )
  {
    TRACE("Send successful");
  }
  else
  {
    TRACE("Send failed");
  }

  radio.powerDown();
  digitalWrite(RF_IO_PWR_PIN, LOW);
}

//****************************************************************
// set system into the sleep state
// system wakes up when wtchdog is timed out
void RF24_system_sleep()
{
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(sleep_mode); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

// the setup function runs once when you press reset or power the board
void setup()
{
  TRACE_GENERIC(Serial.begin(115200));

  //source vcc to RF through IO pin
  pinMode(RF_IO_PWR_PIN, OUTPUT);

  setup_watchdog(9);//8 seconds
  TRACE_GENERIC(setup_watchdog(8));//debug at 4secs

  analogReference(EXTERNAL);

  pinMode(relayPin, OUTPUT); //what is pin7?
  digitalWrite(relayPin, 0);
  TRACE("setting up");
}

void sampleTemp()
{
	temp = ntc.getTemp();
}

void turnCompressorOn(){
  state=HIGH;
  digitalWrite(relayPin,state);
}

void turnCompressorOff(){
  state=LOW;
  digitalWrite(relayPin,state);
}

boolean isCompressorOff(){
  return (state==LOW);
}

boolean isCompressorOn(){
  return !isCompressorOff();
}

void freezerControl()
{
	//freezer control
#define HYST 1.5
#define TEMP_SETPOINT -18
	//sample temp every 8 second - 2 filters
	// one 5min to ignore doors and for compressor control
	sampleTemp();
	if (temp>TEMP_SETPOINT+HYST && isCompressorOff())
	{
		turnCompressorOn();
	}
	else if (temp<TEMP_SETPOINT && isCompressorOn())
	{
		turnCompressorOff();
	}
}
// the loop function runs over and over again forever
void loop()
{
#define MINUTES 60/8
	int timeLimit = 5*MINUTES;
	TRACE_GENERIC(timeLimit = 0);
	timeLimit = 0;
	if (WakeUpTime > timeLimit)
	{
		WakeUpTime = 0;

		freezerControl();
		TRACE(temp);
		TRACE(state);

		//RadioStuffBelow
		sendOverRadio();

		TRACE("woke up");
		TRACE(temp);
		TRACE(state);
	}
	RF24_system_sleep();

}

