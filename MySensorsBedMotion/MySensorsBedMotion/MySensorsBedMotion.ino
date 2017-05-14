// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_RF24_PA_LEVEL RF24_PA_MIN
//#define MY_RADIO_RFM69

#define LED_PIN 6
#define MOTION_PIN_1 3  //the pin the first motion sensor is attached to
#define MOTION_PIN_2 7  //the pin the second motion sensor is attached to

#define FADE_AMOUNT 1    // how many points to fade the LED by
#define FADE_DELAY 100 //delay between fade steps, in MS
#define MAX_ON_TIME 5000 //Time to wait before fading down after no motion is detected anymore

#include <SPI.h>
#include <MySensors.h>  

#define CHILD_ID_LIGHT 1
#define CHILD_ID_MOTION 2

#define EPROM_LIGHT_STATE 1
#define EPROM_DIMMER_LEVEL 2

#define LIGHT_OFF 0
#define LIGHT_ON 1

#define SN "Motion trgr dimlight"
#define SV "1.0"

//States for the state machine
#define STATE_IDLE 0
#define STATE_TRIPPED 1
#define STATE_FADING_UP 2
#define STATE_FADED_MAX 3
#define STATE_FADING_DOWN 4

int state = STATE_IDLE;
unsigned long reachedFadeMaxAt = 0;

bool prevTripped = false;

int MaxDimValue = 255; //The maximum value to fade the light to. Between 0..255

int CurrentDimValue = 0;
int DesiredDimValue = 0;

MyMessage lightMsg(CHILD_ID_LIGHT, V_LIGHT);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);
MyMessage motionMsg(CHILD_ID_MOTION, V_TRIPPED);

void setup()  
{ 
  //Retreive our last light state from the eprom
  int LightState=loadState(EPROM_LIGHT_STATE); 
  MaxDimValue=loadState(EPROM_DIMMER_LEVEL);  
  
  if(MaxDimValue > 255) MaxDimValue = 255;
  if(MaxDimValue < 0) MaxDimValue = 0;

  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTION_PIN_1, INPUT);
  pinMode(MOTION_PIN_2, INPUT);

  analogWrite(LED_PIN, 255);
  wait(100);
  analogWrite(LED_PIN, 0);
  wait(100);
  analogWrite(LED_PIN, 255);
  wait(100);
  analogWrite(LED_PIN, 0);
  wait(100);
  analogWrite(LED_PIN, 255);
  wait(100);
  analogWrite(LED_PIN, 0);
  wait(100);
  analogWrite(LED_PIN, 255);
  wait(100);
  analogWrite(LED_PIN, 0);
  wait(100);
  analogWrite(LED_PIN, 255);
  wait(100);
  analogWrite(LED_PIN, 0);
  wait(100);
  analogWrite(LED_PIN, 255);
  wait(100);
  analogWrite(LED_PIN, 0);
  wait(100);

  Serial.println( "Node ready to receive messages..." );  
  
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo(SN, SV);

  present(CHILD_ID_LIGHT, S_DIMMER);
  present(CHILD_ID_MOTION, S_MOTION);
  SendCurrentState2Controller();
}

bool atDesiredDimValue()
{
  return (abs(CurrentDimValue - DesiredDimValue) < FADE_AMOUNT);
}

void fadeToLevel() {
  
  
  if (!atDesiredDimValue()) {
    int delta = ( DesiredDimValue - CurrentDimValue ) < 0 ? -1 : 1;

    CurrentDimValue += delta * FADE_AMOUNT;
    Serial.print("analog write: ");
    Serial.println(CurrentDimValue);
    analogWrite(LED_PIN, CurrentDimValue);
    wait(FADE_DELAY);
  } else if(DesiredDimValue == 0)
  {
    CurrentDimValue = 0;
    analogWrite(LED_PIN, CurrentDimValue);
    
  }
}

void loop()      
{
  bool tripped1 = digitalRead(MOTION_PIN_1) == HIGH;
  bool tripped2 = digitalRead(MOTION_PIN_2) == HIGH;
  bool tripped = tripped1 || tripped2;
  
  if(tripped != prevTripped)
  {
    send(motionMsg.set(tripped?"1":"0"));  // Send tripped value to gw
    prevTripped = tripped;
  }

  //Serial.print("Tripped1 = ");
  //Serial.print(tripped1);
  //Serial.print(", Tripped2 = ");
  //Serial.print(tripped2);
  //Serial.print(", State = ");
  //Serial.print(state);
  //Serial.print(", Tripped = ");
  //Serial.print(tripped);
  //Serial.print(", CurrentDimValue = ");
  //Serial.print(CurrentDimValue);
  //Serial.print(", DesiredDimValue = ");
  //Serial.print(DesiredDimValue);
  Serial.print(", MaxDimValue = ");
  Serial.println(MaxDimValue);

  switch(state)
  {
    case STATE_IDLE:
      if(tripped)
      {
        state = STATE_TRIPPED;
      }
    break;
    case STATE_TRIPPED:
      DesiredDimValue = MaxDimValue;
      state = STATE_FADING_UP;
    break;
    case STATE_FADING_UP:
      if(atDesiredDimValue())
      {
        reachedFadeMaxAt = millis();
        state = STATE_FADED_MAX;
      }
    break;
    case STATE_FADED_MAX:
      if(tripped) {
        reachedFadeMaxAt = millis();
      }

      if(millis() - reachedFadeMaxAt > MAX_ON_TIME)
      {
        DesiredDimValue = 0;
        state = STATE_FADING_DOWN;
      }
    break;
    case STATE_FADING_DOWN:
      if(tripped)
      {
        DesiredDimValue = MaxDimValue;
        state = STATE_FADING_UP;
      }
      if(atDesiredDimValue())
      {
        state = STATE_IDLE;
      }
    break;
  }
  
  fadeToLevel();
}

void receive(const MyMessage &message)
{
  if (message.type == V_LIGHT) {
    Serial.println( "V_LIGHT command received..." );
    int lstate= atoi( message.data );
    Serial.print("light command: ");
    Serial.println(lstate);
    if(lstate == LIGHT_OFF)
    {
      Serial.println("LIGHT_OFF received!");
      MaxDimValue = 0;
    } else {
      Serial.println("LIGHT_ON received. Ignored!");
    }
  }
  else if (message.type == V_DIMMER) {
    Serial.println( "V_DIMMER command received..." );  
    int dimvalue= atoi( message.data );
    if ((dimvalue<0)||(dimvalue>100)) {
      Serial.println( "V_DIMMER data invalid (should be 0..100)" );
      return;
    } else {
      Serial.print("V_DIMMER value: ");
      Serial.println(dimvalue);
    }
    
    MaxDimValue=dimvalue*2.55 / 3;
    saveState(EPROM_DIMMER_LEVEL, MaxDimValue);
  }
  else {
    Serial.println( "Invalid command received..." );  
    return;
  }

  SendCurrentState2Controller();
}

void SendCurrentState2Controller()
{
    Serial.print("Sending dimmer state ");
    int dimValue = MaxDimValue / 2.55 * 3;
    Serial.println(dimValue);
    send(dimmerMsg.set(dimValue));
}

