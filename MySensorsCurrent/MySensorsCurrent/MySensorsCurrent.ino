//AC current measurement inspired by: http://henrysbench.capnfatz.com/henrys-bench/arduino-current-measurements/acs712-arduino-ac-current-tutorial/

// Enable debug prints to serial monitor
//#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

//This node is a repeater
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>  

#define CURRENT_SENSOR_ANALOG_PIN A5
#define CHILD_ID 0

unsigned long SLEEP_TIME = 5000; // Sleep time between reads (in milliseconds)
uint32_t lastTransmitTime;

MyMessage msg(CHILD_ID, V_CURRENT);

double LastArms = 0;
const int mVperAmp = 66; // use 100 for 20A Module and 66 for 30A Module

////Intermediate variables
//Peak-to-peak voltage
double Vpp = 0;
//RMS voltage
double Vrms = 0;
//RMS current
double Arms = 0;

void setup() {
  lastTransmitTime = millis();
}

void presentation() {
  present(CHILD_ID, S_MULTIMETER);
}

float getVpp()
{
  float result;

  int readValue; //Value read from the sensor
  int maxValue = 0; //Maximum value found during sampling
  int minValue = 1024; //Minimum value found during sampling

  uint32_t start_time = millis();
  while((millis()-start_time) < 1000) //sample for 1 second
  {
    readValue = analogRead(CURRENT_SENSOR_ANALOG_PIN);

    //see if we found a new maximum
    if (readValue > maxValue)
    {
      maxValue = readValue;
    }
    //see if we found a new minimum
    if(readValue < minValue)
    {
      minValue = readValue;
    }
  }

  //We should now have our peak values. Convert those into a voltage and return.
  result = ((maxValue - minValue) * 5.0) / 1024.0;
  return result;
}

void loop() {
  //Transmit every SLEEP_TIME milliseconds. Since this is a repeater node, we can't use sleep here.
  if(millis()-lastTransmitTime > SLEEP_TIME)
  {
    Vpp = getVpp();
    Vrms = (Vpp/2.0) * 0.70710678118654752440084436210485; //Multiply by 0.5*sqrt(2) to get RMS value
    Arms = (Vrms * 1000) / mVperAmp;
    
    #ifdef MY_DEBUG
    Serial.print(Vpp);
    Serial.println(" Vpp");
    
    Serial.print(Vrms);
    Serial.println(" Vrms");
    
    Serial.print(Arms);
    Serial.println(" Amps RMS");
    #endif
  
    double difference = Arms - LastArms;
    if(abs(difference) > 0.1)
    {
      send(msg.set(Arms,1));
      LastArms = Arms;
    }
    lastTransmitTime = millis();
  }
}
