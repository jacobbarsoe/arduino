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
RF24 radio(9,10); // Set up nRF24L01 radio on SPI pin for CE, CSN
const uint64_t pipes[2] = { 0xF0F0F0F0E2LL, 0x7365727631LL };
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

int getTemperature();

void sendOverRadio()
{
  char outBuffer[16];

  digitalWrite(RF_IO_PWR_PIN, HIGH);
  delay(1);
  setup_radio();
  TRACE("setup");
  radio.powerUp();
  TRACE("powerup");

  // Append the hex nodeID to the beginning of the payload
  sprintf(outBuffer,"%2X,%03d,%04d,%04d",nodeID,++counter,getTemperature(),readVcc());

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



#endif /* RADIO_H_ */
