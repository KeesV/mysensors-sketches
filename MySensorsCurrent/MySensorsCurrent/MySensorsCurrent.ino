// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h>  

#define CURRENT_SENSOR_ANALOG_PIN A5
#define CHILD_ID 0

unsigned long SLEEP_TIME = 5000; // Sleep time between reads (in milliseconds)

MyMessage msg(CHILD_ID, V_CURRENT);
float lastCurrentLevel = 0;

int calVal = 512;
int mVpA = 61;

void setup() {
}

void presentation() {
  present(CHILD_ID, S_MULTIMETER);
}

void loop() {
  int val = analogRead(CURRENT_SENSOR_ANALOG_PIN);
  int valAdj = val - calVal;
  float milliVolts = ((valAdj * 5.00) / 1024) * 1000;
  float Amps = milliVolts / mVpA;
  float absAmps = abs(Amps);
  
  #ifdef MY_DEBUG
  Serial.println(val);
  Serial.println(milliVolts);
  Serial.println(Amps);
  Serial.println(absAmps);
  #endif

  float difference = lastCurrentLevel - absAmps;
  if(abs(difference) > 0.5)
  {
    send(msg.set(absAmps,1));
    lastCurrentLevel = absAmps;
  }
  
  sleep(SLEEP_TIME);
}
