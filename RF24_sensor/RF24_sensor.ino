/*
  Send data over RF24

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
#include "ReadVcc.h"

#define JABK_DEBUG

#ifdef JABK_DEBUG
#define TRACE(x) Serial.println(x)
#define TRACE_GENERIC(x) x
#else
#define TRACE(x)
#define TRACE_GENERIC(x)
#endif

//pins
#define TEST_TOGGLE_PIN     5
#define RF_IO_PWR_PIN       8
#define RF_IO_GND_PIN       7

//Globals
RF24 radio(9,10); // Set up nRF24L01 radio on SPI pin for CE, CSN
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0x7365727631LL };
uint16_t nodeID = 1;
char receivePayload[32];

volatile int f_wdt;
volatile int payload = 0;
uint8_t counter = 0;
volatile int flag = 0;
int batt = 0;

volatile int sleep_mode = SLEEP_MODE_PWR_DOWN;

//****************************************************************
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect)
{
	++f_wdt;
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

  delay(1);
  setup_radio();
  TRACE("setup");
  radio.powerUp();
  TRACE("powerup");

  // Append the hex nodeID to the beginning of the payload
  sprintf(outBuffer,"%2X,%03d,%04d,%04d",nodeID,++counter, batt, payload);

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
  pinMode(RF_IO_GND_PIN, OUTPUT);
  digitalWrite(RF_IO_PWR_PIN, LOW);
  digitalWrite(RF_IO_GND_PIN, LOW);

  setup_watchdog(9);//8 seconds
  TRACE_GENERIC(setup_watchdog(8));//debug at 4secs

  pinMode(TEST_TOGGLE_PIN, OUTPUT);

  analogReference(INTERNAL);

  //power reduction
  // boards.txt: change to mini.menu.cpu.atmega328.bootloader.extended_fuses=0x07
  //enable tim0, spi and adc
  PRR = 0xFF & ~((1<<PRTIM0) | (1<<PRSPI) | (1<<PRADC) TRACE_GENERIC(| (1<<PRUSART0)) );
}



// the loop function runs over and over again forever
void loop()
{
  int timeLimit = 5*60/8;
  TRACE_GENERIC(timeLimit = 0);
  if (f_wdt > timeLimit)
  {
    f_wdt = 0;
    digitalWrite(RF_IO_PWR_PIN, HIGH);
    sendOverRadio();
    digitalWrite(RF_IO_PWR_PIN, LOW);

    TRACE("woke up");
    TRACE(payload);
    TRACE(batt);

    // we actually send the old batt value. to be
    // able to ensure the ADC has settled
    // and we still need to wait 10ms
    delay(10);
    //batt = readVcc();
  }
  TRACE_GENERIC(digitalWrite(TEST_TOGGLE_PIN, LOW)); //measured round trip time to 19ms
  RF24_system_sleep();
  TRACE_GENERIC(digitalWrite(TEST_TOGGLE_PIN, HIGH));

}

