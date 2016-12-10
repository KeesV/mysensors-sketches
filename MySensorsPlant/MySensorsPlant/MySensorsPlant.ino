// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <MySensors.h>  

#define SENS_PIN A1

#define CHILD_ID_HUM 0

MyMessage msgHum(CHILD_ID_HUM, V_HUM);

float lastHum = 0.0;
uint8_t lastTransmitTime;

#define SLEEP_TIME 60000 //10 minutes
#define MAX_SLEEP_TIME 3600000 //1 hour

void before()
{
  analogReference(DEFAULT);  
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Plant Hum Sensor, Batt Pwr", "1.1");

  present(CHILD_ID_HUM, S_HUM);
}

void setup() {
  lastTransmitTime = millis();
}

void loop() {
  float analogValue = analogRead(SENS_PIN);
  float humPercent = 100 - (analogValue * 100 / 1024);
  Serial.print("Humidity: ");
  Serial.println(humPercent);

  float diff = abs(lastHum - humPercent);
  if(diff > 0 || millis() - lastTransmitTime > MAX_SLEEP_TIME)
  {
    send(msgHum.set(humPercent,1));
    lastTransmitTime = millis();
    lastHum = humPercent;
  }

  sleep(SLEEP_TIME);
}
