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

////Intermediate variables
const unsigned long sampleTime = 100000UL;
const unsigned long numSamples = 250UL;
const unsigned long sampleInterval = sampleTime / numSamples;
const int adc_zero = 513;
const float rms_correction = -0.05;

void setup() {
  lastTransmitTime = millis();
}

void presentation() {
  present(CHILD_ID, S_MULTIMETER);
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
  //Transmit every SLEEP_TIME milliseconds. Since this is a repeater node, we can't use sleep here.
  if(millis()-lastTransmitTime > SLEEP_TIME)
  {
    float rmsCurrent = getRms();
    #ifdef MY_DEBUG
    Serial.print("RMS: ");
    Serial.println(rmsCurrent);
    #endif
    
    send(msg.set(rmsCurrent,1));   
    lastTransmitTime = millis();
  }
}
