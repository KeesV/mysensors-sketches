//AC current measurement inspired by: http://henrysbench.capnfatz.com/henrys-bench/arduino-current-measurements/acs712-arduino-ac-current-tutorial/

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

//This node is a repeater
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>  

#define CURRENT_SENSOR_ANALOG_PIN A5
#define CHILD_ID_CURRENT 0
#define CHILD_ID_ONOFF 1

#define CURRENT_THRESHOLD 0.01

unsigned long SLEEP_TIME = 5000; // Sleep time between reads (in milliseconds)
uint32_t lastTransmitTime;

MyMessage msg_current(CHILD_ID_CURRENT, V_CURRENT);
MyMessage msg_onoff(CHILD_ID_ONOFF, V_STATUS);

////Intermediate variables
const unsigned long sampleTime = 100000UL;
const unsigned long numSamples = 250UL;
const unsigned long sampleInterval = sampleTime / numSamples;
const int adc_zero = 513;
const float rms_correction = -0.05;

bool lastStatus = false;
bool prevStatus = false;
int counter = 0;

void setup() {
  lastTransmitTime = millis();

  float rmsCurrent = getRms();
  lastStatus = rmsCurrent > CURRENT_THRESHOLD ? true : false;
}

void presentation() {
  present(CHILD_ID_CURRENT, S_MULTIMETER);
  present(CHILD_ID_ONOFF, S_BINARY);
}

float getRms()
{
  unsigned long currentAcc = 0;
  unsigned int count = 0;
  unsigned long prevMicros = micros() - sampleInterval;
  while ( count < numSamples)
  {
    if(micros() - prevMicros >= sampleInterval)
    {
      int adc_raw = analogRead(CURRENT_SENSOR_ANALOG_PIN) - adc_zero;
      currentAcc += (unsigned long)(adc_raw * adc_raw);
      ++count;
      prevMicros += sampleInterval;
    }
  }
  float rms = abs(sqrt((float)currentAcc/(float)numSamples) * (75.7576 / 1024.0) + rms_correction);
  return rms;
}

void loop() {
  float rmsCurrent = getRms();
  bool currentStatus = rmsCurrent > CURRENT_THRESHOLD ? true : false;

  if(currentStatus == prevStatus)
  {
    counter++;
  } else {
    counter = 0;
  }
  prevStatus = currentStatus;

  #ifdef MY_DEBUG
  Serial.print("Values: ");
  Serial.print(rmsCurrent);
  Serial.print("   ");
  Serial.print(currentStatus);
  Serial.print("   ");
  Serial.println(counter);
  #endif

  //Transmit on/off status if it has changed
  if ((currentStatus != lastStatus && counter > 15) || millis()-lastTransmitTime > SLEEP_TIME)
  {
    send(msg_onoff.set(currentStatus));
    lastStatus = currentStatus;
  }
  
  //Transmit current every SLEEP_TIME milliseconds. Since this is a repeater node, we can't use sleep here.
  if(millis()-lastTransmitTime > SLEEP_TIME)
  {   
    send(msg_current.set(rmsCurrent,1));   
    lastTransmitTime = millis();
  }
}
