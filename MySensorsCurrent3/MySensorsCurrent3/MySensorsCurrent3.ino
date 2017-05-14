#define PIN_LED_STATUS 4 
#define PIN_LED_TRX 5
#define PIN_CURRENT_SENSOR A5
#define PIN_TEMP_SENSOR 3

#define LED_ON LOW
#define LED_OFF HIGH

//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define CHILD_ID_CURRENT 0
#define CHILD_ID_ONOFF 1
#define CHILD_ID_TEMP 2

OneWire oneWire(PIN_TEMP_SENSOR); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature tempSensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 

MyMessage msg_current(CHILD_ID_CURRENT, V_CURRENT);
MyMessage msg_onoff(CHILD_ID_ONOFF, V_STATUS);
MyMessage msg_temp(CHILD_ID_TEMP, V_TEMP);

////Intermediate variables
const unsigned long sampleTime = 100000UL;
const unsigned long numSamples = 500UL;
const unsigned long sampleInterval = sampleTime / numSamples;
const int adc_zero = 513;
const float rms_correction = -0.07;

unsigned long SLEEP_TIME = 20000; // Sleep time between reads (in milliseconds)
uint32_t lastTransmitTime;
bool lastStatus;

#define CURRENT_THRESHOLD 0.025 //Below this is considered Off
#define ALMOST_OFF_TIME 300000 //5 minutes = 300000

#define STATE_LOAD_OFF 0
#define STATE_LOAD_ON 1
#define STATE_LOAD_ALMOST_OFF 2
int state = STATE_LOAD_OFF;
unsigned long state_almost_off_time = 0;

void before() {
  tempSensors.begin();
}

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_LED_TRX, OUTPUT);

  tempSensors.setWaitForConversion(false);
  
  digitalWrite(PIN_LED_STATUS, LED_ON);
  digitalWrite(PIN_LED_TRX, LED_OFF);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_ON);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_ON);
  digitalWrite(PIN_LED_TRX, LED_OFF);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_ON);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_ON);
  digitalWrite(PIN_LED_TRX, LED_OFF);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_ON);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_ON);
  digitalWrite(PIN_LED_TRX, LED_OFF);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_ON);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_ON);
  digitalWrite(PIN_LED_TRX, LED_OFF);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_ON);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_ON);
  digitalWrite(PIN_LED_TRX, LED_OFF);
  wait(100);
  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_ON);
  wait(100);

  digitalWrite(PIN_LED_STATUS, LED_OFF);
  digitalWrite(PIN_LED_TRX, LED_OFF);

  lastTransmitTime = millis();
  float rmsCurrent = getRms();
  lastStatus = true;

  Serial.println("Current sensor started!");
}

void presentation() {
  sendSketchInfo("Current Sensor", "1.0");
  present(CHILD_ID_CURRENT, S_MULTIMETER);
  present(CHILD_ID_ONOFF, S_BINARY);
  present(CHILD_ID_TEMP, S_TEMP);
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
      int adc_raw = analogRead(PIN_CURRENT_SENSOR) - adc_zero;
      currentAcc += (unsigned long)(adc_raw * adc_raw);
      ++count;
      prevMicros += sampleInterval;
    }
  }
  float rms = abs(sqrt((float)currentAcc/(float)numSamples) * (75.7576 / 1024.0) + rms_correction);
  return rms;
}

void send_load_status(bool status)
{
  Serial.print("Sending load status: ");
  Serial.println(status);
  digitalWrite(PIN_LED_TRX, LED_ON);
  send(msg_onoff.set(status)); 
  digitalWrite(PIN_LED_TRX, LED_OFF);
}

void send_current_status(float current)
{
  Serial.print("Sending load current: ");
  Serial.println(current);
  digitalWrite(PIN_LED_TRX, LED_ON);
  send(msg_current.set(current,1));
  digitalWrite(PIN_LED_TRX, LED_OFF);
}

void send_temp_status(float temp)
{
  Serial.print("Sending temperature: ");
  Serial.println(temp);
  digitalWrite(PIN_LED_TRX, LED_ON);
  send(msg_temp.setSensor(0).set(temp,1));
  digitalWrite(PIN_LED_TRX, LED_OFF);
}

void loop() {
  //Fetch current
  float rmsCurrent = getRms();
  //Serial.print("RMS current: ");
  //Serial.println(rmsCurrent);
  
  //Transmit current and temperature every SLEEP_TIME milliseconds. Since this is a repeater node, we can't use sleep here.
  if(millis()-lastTransmitTime > SLEEP_TIME)
  { 
    send_current_status(rmsCurrent);

    // Fetch temperatures from Dallas sensors
    tempSensors.requestTemperatures();
    
    // query conversion time and wait until conversion completed
    int16_t conversionTime = tempSensors.millisToWaitForConversion(tempSensors.getResolution());
    wait(conversionTime);
  
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>(( tempSensors.getTempCByIndex(0)) * 10.)) / 10.;
    if (temperature != -127.00 && temperature != 85.00) {
      send_temp_status(temperature);
    }
    
    lastTransmitTime = millis();
  }

  switch(state)
  {
    case STATE_LOAD_OFF:
      if(rmsCurrent > CURRENT_THRESHOLD)
      {
        state = STATE_LOAD_ON;
        send_load_status(true);
        #ifdef MY_DEBUG
        Serial.print("Current is: ");
        Serial.println(rmsCurrent);
        Serial.println("State: OFF -> ON");
        #endif
      }
    break;

    case STATE_LOAD_ON:
      if(rmsCurrent <= CURRENT_THRESHOLD)
      {
        state = STATE_LOAD_ALMOST_OFF;
        state_almost_off_time = millis();
        #ifdef MY_DEBUG
        Serial.print("Current is: ");
        Serial.println(rmsCurrent);
        Serial.println("State: ON -> ALMOST OFF");
        #endif
      }
    break;

    case STATE_LOAD_ALMOST_OFF:
      if(rmsCurrent > CURRENT_THRESHOLD)
      {
        state = STATE_LOAD_ON;
        #ifdef MY_DEBUG
        Serial.print("Current is: ");
        Serial.println(rmsCurrent);
        Serial.println("State: ALMOST OFF -> ON");
        #endif
      } else {
        #ifdef MY_DEBUG
        Serial.print("Almost off time: ");
        Serial.println(millis() - state_almost_off_time);
        #endif
        if(millis() - state_almost_off_time > ALMOST_OFF_TIME)
        {
          state = STATE_LOAD_OFF;
          send_load_status(false);
          #ifdef MY_DEBUG
          Serial.println("State: ALMOST OFF -> OFF");
          #endif MY_DEBUG
        }
      }
    break;
  }

  //Set LED status according to state
  if(state == STATE_LOAD_ON || state == STATE_LOAD_ALMOST_OFF)
  {
    digitalWrite(PIN_LED_STATUS, LED_ON);
  } else {
    digitalWrite(PIN_LED_STATUS, LED_OFF);
  }
}
