/* Short explanation
Create an instance of the Sunrise class
Pass month and day to Rise() or Set(), returning minutes since midnight of the event
Read the hour and minute if desired
See the website for more detailed instructions
http://www.swfltek.com/arduino/sunrise
*/

#include <Sunrise.h>

// create a Sunrise object
#include "WProgram.h"
void setup();
void loop();
Sunrise mySunrise(39,-9,0);
//Lisbon, Portugal, Europe - Latitude/Longitude and Timezone 	38.79/-9.12, 0

void setup(){
  Serial.begin(9600);
  mySunrise.Actual(); 
}

void loop(){
  byte h,m;
  int t;
  
  t=mySunrise.Rise(12,30); // month,date - january=1 ;  t= minutes past midnight of sunrise (6 am would be 360)
  if(t>=0){
    h=mySunrise.Hour();
    m=mySunrise.Minute();
    Serial.print("The sun rises on June 30 at ");
    Serial.print(h, DEC);
    Serial.print(":");
    if(m<10) Serial.print("0");
    Serial.println(m,DEC);
  }
  else{ // 
    Serial.println("There are either penguins or polar bears around here!");
  }
  
    t=mySunrise.Set(12,30); // month,date - january=1 ; t= minutes past midnight of sunrise (6 am would be 360)
  if(t>=0){
    h=mySunrise.Hour();
    m=mySunrise.Minute();
    Serial.print(" | The sun sets on June 30 at ");
    Serial.print(h, DEC);
    Serial.print(":");
    if(m<10) Serial.print("0");
    Serial.println(m,DEC);
  }
  else{ // 
    Serial.println("There are either penguins or polar bears around here!");
  }
  while(true){}
  
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

