// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 8 on the Arduino
#define ONE_WIRE_BUS 8

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{
  // start serial port
  Serial.begin(115200);
  // Start up the library
  sensors.begin();
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);

}

/*
 * Main function, get and show the temperature
 */
void loop(void)
{
	static unsigned long oldMillis = 0;
#define SAMPLE_TIME	60000
	if (oldMillis + SAMPLE_TIME < millis())
	{
	  oldMillis = millis();
	  // call sensors.requestTemperatures() to issue a global temperature
	  // request to all devices on the bus
	  sensors.requestTemperatures(); // Send the command to get temperatures

	  // After we got the temperatures, we can print them here.
	  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
	  Serial.print(millis()/1000);
	  Serial.print(", ");
	  Serial.print(sensors.getTempCByIndex(0));
	  Serial.print(", ");
	  Serial.print(sensors.getTempCByIndex(1));
	  Serial.print(", ");
	  Serial.print(digitalRead(10));
	  for(int i=0; i<6; i++)
	  {
		  Serial.print(", ");
		  Serial.print(digitalRead(2+i));
	  }
	  Serial.println();
	}
}
