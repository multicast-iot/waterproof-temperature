#include <OneWire.h>
#include <DallasTemperature.h>

// Board constants
#define PIN_SENSOR_0      12 // pin in which the temperature sensor is connected
#define PIN_SENSOR_1      13 // pin in which the temperature sensor is connected
#define PIN_LED           14
#define PIN_BOARD_BUTTON   0

#define ONE_WIRE_BUS PIN_SENSOR_0

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  
  Serial.println("Starting sensors ...");
  sensors.begin();
  Serial.println("Sensors started");
  
}

void loop(void)
{ 
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  float tempC = sensors.getTempCByIndex(0);

  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
  } 
  else
  {
    Serial.println("Error: Could not read temperature data");
  }
}
