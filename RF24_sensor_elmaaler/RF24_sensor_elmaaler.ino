/*
  Send el data over RF24 
 
 go to deep sleep in between transmits (one way)
 */

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


volatile int f_wdt;
volatile int forbrug = 0;
uint8_t counter = 0;
volatile int flag = 0;


//#define JABK_DEBUG
//#define JABK_SERIAL_DEBUG

#define TEST_TOGGLE_PIN     5
#define RF_IO_PWR_PIN       81
#define SAMPLE_BATT_VCC_PIN 4
#define EL_IRQ_PIN          2
#define EL_VCC_PIN          3
#define SAMPLE_BATT_ADC     3

// NRF24L01
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
// Set up nRF24L01 radio on SPI pin for CE, CSN
RF24 radio(9,10);
// Example below using pipe5 for writing
const uint64_t pipes[2] = { 
  0xF0F0F0F0E1LL, 0x7365727631LL };

uint16_t nodeID = 1;

char receivePayload[32];

void setup_radio() {
  //CONFIGURE RADIO
  radio.begin();
  // Enable this seems to work better
  radio.enableDynamicPayloads();
  radio.setAutoAck(1);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setChannel(70);
  radio.setRetries(15,15);
  radio.setCRCLength(RF24_CRC_16);

  radio.openWritingPipe(pipes[0]);
  //radio.openReadingPipe(1,pipes[1]);
}

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
}

void isr() {
  //delay(1); //debounce 5ms
  if (digitalRead(EL_IRQ_PIN) == 0) {
    while (digitalRead(EL_IRQ_PIN) == 0) //wait up
    {
      delay(1);
    }
    forbrug++;
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
#ifdef JABK_SERIAL_DEBUG
  Serial.begin(115200);
#endif

  //source vcc to RF through IO pin
  pinMode(RF_IO_PWR_PIN, OUTPUT);
  
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

#ifdef JABK_DEBUG
  setup_watchdog(8);//9debug at 6
#else
  setup_watchdog(9);//9
#endif
  pinMode(EL_IRQ_PIN, INPUT);   //d2
  pinMode(EL_VCC_PIN, OUTPUT);   //d3 //d2 vcc
  pinMode(TEST_TOGGLE_PIN, OUTPUT);   //d5
  digitalWrite(EL_VCC_PIN, HIGH);
  pinMode(SAMPLE_BATT_VCC_PIN, OUTPUT);   //d4 //samplebatt
  digitalWrite(SAMPLE_BATT_VCC_PIN, HIGH);
  attachInterrupt(0, isr, LOW);
  analogReference(INTERNAL);
}

int batt = 0;
void sampleADCBatt() {
  batt = analogRead(SAMPLE_BATT_ADC);
   digitalWrite(SAMPLE_BATT_VCC_PIN, LOW);
}

void sendOverRadio() {
  digitalWrite(RF_IO_PWR_PIN, HIGH);
  delay(1);
  setup_radio();
#ifdef JABK_SERIAL_DEBUG
  Serial.println("setup");
#endif
  radio.powerUp();
#ifdef JABK_SERIAL_DEBUG
  Serial.println("powerup");
#endif

  //prepare 
  uint8_t data1 = 0;
  char temp[5];
  char outBuffer[16];

  outBuffer[16]=0;
  //data1 is a counter
  data1 = counter++;

  // Append the hex nodeID to the beginning of the payload
  sprintf(outBuffer,"%2X",nodeID);
  strcat(outBuffer,",");
  sprintf(temp,"%03d",data1);
  strcat(outBuffer,temp);
  strcat(outBuffer,",");
  sprintf(temp,"%04d",forbrug);
  strcat(outBuffer,temp);
  strcat(outBuffer,",");
  sprintf(temp,"%04d",batt);
  strcat(outBuffer,temp);

  // Stop listening and write to radio
  radio.stopListening();
#ifdef JABK_SERIAL_DEBUG
  Serial.println(outBuffer);
  Serial.println("stoplistening");
#endif
  // Send to hub
  if ( radio.write( outBuffer, strlen(outBuffer)) ) {
#ifdef JABK_SERIAL_DEBUG
    Serial.println("Send successful");
#endif
  } 
  else {
#ifdef JABK_SERIAL_DEBUG
    Serial.println("Send failed");
#endif
  }

  radio.powerDown();
  digitalWrite(RF_IO_PWR_PIN, LOW);
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  attachInterrupt(0, isr, LOW);
  sleep_mode();                        // System sleeps here
  detachInterrupt(0);
  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

//****************************************************************  
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
  f_wdt++;  // set global flag  sei();
}


// the loop function runs over and over again forever
void loop() {
#ifndef JABK_DEBUG
  if (f_wdt > 5*60/8) {
#else
  if (f_wdt > 0) {
#endif
    f_wdt = 0;
    digitalWrite(SAMPLE_BATT_VCC_PIN, HIGH);
    sendOverRadio();    
    forbrug = 0;
#ifdef JABK_SERIAL_DEBUG
    Serial.println("woke up");   // turn the LED on (HIGH is the voltage level)
    Serial.println(forbrug);
    Serial.println(batt);
#endif
    delay(10);
    sampleADCBatt();    
  }
#ifdef JABK_DEBUG
  digitalWrite(TEST_TOGGLE_PIN, LOW);
#endif
  system_sleep();
#ifdef JABK_DEBUG
  digitalWrite(TEST_TOGGLE_PIN, HIGH);
#endif
}

