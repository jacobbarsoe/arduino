/*
 * Radio.h
 *
 *  Created on: Oct 2, 2015
 *      Author: jabk
 */

#ifndef RADIO_H_
#define RADIO_H_

// NRF24L01
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

//Globals
RF24 radio(RF_CE,RF_CSN); // Set up nRF24L01 radio on SPI pin for CE, CSN
const uint64_t pipes[2] = { 0xF0F0F0F0E2LL, 0xF0F0F0F0D2LL };
uint16_t nodeID = 2;
char receivePayload[32];
uint8_t counter = 0;

int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
     ADMUX = _BV(MUX5) | _BV(MUX0) ;
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  int result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void setup_radio(bool openForReading)
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
  if (openForReading)
	  radio.openReadingPipe(1,pipes[1]);
}

int getTemperature();
time_t syncClockToRTC();
void setRTCVCC(int on);

struct Payload
{
	char nodeID;
	char counter;
	char openOrClosed;
	char receiveEnabled;
	int16_t  temperature;
	int16_t  batteryVoltage;
	long time;
	Today openTime;
	Today closeTime;
} __attribute__ ((packed));

void receiveTime()
{
	time_t syncTime;
	while(!radio.available()) // If an ack with payload was received
	{
		delay(5);
	}
	TRACE("received Data");
	radio.read( &syncTime, sizeof(time_t));     // Read it, and display the response time
	setRTCVCC(1);
	RTC.set(syncTime);
	delay(5);
	setRTCVCC(0);
}

void sendOverRadio(bool receiveEnabled=false)
{
  struct Payload data;

  digitalWrite(RF_IO_PWR_PIN, HIGH);
  delay(1);
  setup_radio(receiveEnabled);
  TRACE("setup");
  radio.powerUp();
  TRACE("powerup");

  // Append the hex nodeID to the beginning of the payload
  data.nodeID = nodeID;
  data.counter = ++counter;
  data.openOrClosed = stepper.currentPosition() > 0 ? 1 : 0;
  data.receiveEnabled = receiveEnabled;
  data.temperature = getTemperature();
  data.batteryVoltage = readVcc();
  data.time = syncClockToRTC();
  data.openTime = nextSunrise;
  data.closeTime = nextSunset;

  // Stop listening and write to radio
  radio.stopListening();
  //TRACE(outBuffer);
  TRACE("stoplistening");

  // Send to hub
  if ( radio.write( &data, sizeof(data)) )
  {
    TRACE("Send successful");

    radio.startListening();
    if (receiveEnabled) //if firstTime - sync clock with gw
    {
	    TRACE("receiveEnabled");
    	receiveTime();
    }
  }
  else
  {
    TRACE("Send failed");
  }

  radio.powerDown();
  digitalWrite(RF_IO_PWR_PIN, LOW);
}



#endif /* RADIO_H_ */
