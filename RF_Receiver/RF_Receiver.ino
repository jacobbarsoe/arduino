
/*
* Getting Started example sketch for nRF24L01+ radios
* This is an example of how to send data from one node to another using data structures
* Updated: Dec 2014 by TMRh20
*/

#include <SPI.h>
#include "RF24.h"


//Globals
RF24 radio(9,10); // Set up nRF24L01 radio on SPI pin for CE, CSN
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0x7365727631LL };

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

  radio.openReadingPipe(1,pipes[0]);
}

void setup() {

  Serial.begin(115200);

  setup_radio();
  radio.startListening();                                    // Now, continue listening

}



void loop()
{

	char buf[16];
                                                               // Grab the response, compare, and send to debugging spew
    uint8_t pipe = 1;
    if (radio.available(&pipe))
    {
    	radio.read( buf, 16 );

    // Spew it
    	Serial.println("read something");
    	Serial.println(buf);
    }

} // Loop

