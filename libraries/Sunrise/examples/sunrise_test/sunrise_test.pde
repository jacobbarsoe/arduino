/* Short explanation
 Create an instance of the Sunrise class
 Pass month and day to Rise() or Set(), returning minutes since midnight of the event
 Read the hour and minute if desired
 See the website for more detailed instructions
 http://www.swfltek.com/arduino/sunrise
 */

#include <Sunrise.h>

// create a Sunrise object
Sunrise mySunrise(27,-82,-5);//Lisbon, Portugal, Europe - Latitude/Longitude and Timezone 	38.79/-9.12, 0

void setup(){
  Serial.begin(9600);
  mySunrise.Astronomical(); //Actual, Civil, Nautical, Astronomical
}

void loop(){
  byte h,m;
  int t;
  // t= minutes past midnight of sunrise (6 am would be 360)
  t=mySunrise.Rise(10,1); // (month,day) - january=1
  if(t>=0){
    h=mySunrise.Hour();
    m=mySunrise.Minute();
    Serial.print("The sun rises on June 30 at ");
    Serial.print(h, DEC);
    Serial.print(":");
    if(m<10) Serial.print("0");
    Serial.println(m,DEC);
  }else{ // 
    Serial.println("There are either penguins or polar bears around here!");
  }
  // t= minutes past midnight of sunrise (6 am would be 360)
  t=mySunrise.Set(10,1); // (month,day) - january=1
  if(t>=0){
    h=mySunrise.Hour();
    m=mySunrise.Minute();
    Serial.print("The sun sets on June 30 at ");
    Serial.print(h, DEC);
    Serial.print(":");
    if(m<10) Serial.print("0");
    Serial.println(m,DEC);
  }else{ // 
    Serial.println("There are either penguins or polar bears around here!");
  }
  // t= minutes past midnight of sunrise (6 am would be 360)
  t=mySunrise.Noon(10,1); // (month,day) - january=1
  if(t>=0){
    h=mySunrise.Hour();
    m=mySunrise.Minute();
    Serial.print("Noon at ");
    Serial.print(h, DEC);
    Serial.print(":");
    if(m<10) Serial.print("0");
    Serial.print(m,DEC);
    Serial.print("   T:");
    Serial.println(sunNoon, DEC);
  }else{ // 
    Serial.println("There are either penguins or polar bears around here!");
  }
}

