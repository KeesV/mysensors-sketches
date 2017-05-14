#define MY_RADIO_NRF24

#include <MyConfig.h>
#include <MySensors.h>

#define PIN_LED 5           // the PWM pin the LED is attached to
#define PIN_MOTION 3  //the pin the motion sensor is attached to

#define SLEEP_TIME 5000

#define FADE_AMOUNT 1    // how many points to fade the LED by
#define MAX_BRIGHTNESS 5 // how bright to make the led
#define FADE_DELAY 200 //delay between fade steps, in MS

int currentLevel = 0;
int requestedLevel = 0;

// the setup routine runs once when you press reset:
void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_MOTION, INPUT);
}

void fadeToLevel() {
  if ( currentLevel != requestedLevel ) {
    int delta = ( requestedLevel - currentLevel ) < 0 ? -1 : 1;

    currentLevel += delta * FADE_AMOUNT;
    Serial.print("analog write: ");
    Serial.println(currentLevel);
    analogWrite(PIN_LED, currentLevel);
    wait(FADE_DELAY);
  }
}

// the loop routine runs over and over again forever:
void loop() {
  bool tripped = digitalRead(PIN_MOTION) == HIGH;
  //Serial.println(tripped);

  if(tripped == true)
  {
    //Turn led on
    requestedLevel = MAX_BRIGHTNESS;
  }

  if(tripped == false)
  {
    requestedLevel = 0;
  }

  fadeToLevel();
  //sleep(digitalPinToInterrupt(PIN_MOTION), CHANGE, SLEEP_TIME);
}
