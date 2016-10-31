/** uses external reference */
#ifndef _NTC_H
#define _NTC_H
#include <math.h>

#define NUMSAMPLES 16
class NTC
{
 public:

  float pin, Rnom, Rseries,Tnom,beta,k;

  NTC(int pin,
      float Rnom,
	  float Rseries,
	  float T0,
	  float beta) :
      pin(pin),
      Rnom(Rnom),
	  Rseries(Rseries),
      Tnom(T0+273.15f),
      beta(beta)
   {
	 	 k = Rnom * exp(-beta / Tnom);
   };

   int samples[NUMSAMPLES];

   float getTemp()
   {
     uint8_t i;
     float average;
 
     // take N samples in a row, with a slight delay
     for (i=0; i< NUMSAMPLES; i++) {
       samples[i] = analogRead(pin);
       delay(10);
     }
 
     // average all the samples out
     average = 0;
     for (i=0; i< NUMSAMPLES; i++) {
       average += samples[i];
     }
     average /= NUMSAMPLES;

     Serial.print("Average analog reading ");
     Serial.println(average);
 

     float r = Rseries  / (1024.0f/average - 1.0f) ;   // # resistance of thermistor
     return (beta / log(r / k)) - 273.15f;   //     # temperature
   };
};
#endif
