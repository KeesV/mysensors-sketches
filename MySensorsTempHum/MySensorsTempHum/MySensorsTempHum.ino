/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Example sketch showing how to send in DS1820B OneWire temperature readings back to the controller
 * http://www.mysensors.org/build/temp
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>  
#include <DHT.h>

// Battery level stuff
int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

// Generic sensor node behavior stuff
unsigned long MIN_SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
unsigned long MAX_SLEEP_TIME = 3600000; //Maximum sleep time, after this it will send a keepAlive message
uint8_t lastHeartbeatTime = 0;

// DHT22 temperature & humidity sensor stuff
#define DHT_DATA_PIN 4
#define SENSOR_TEMP_OFFSET 0

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1

float lastTemp;
float lastHum;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
DHT dht;

bool metric = true;

void before()
{
  // use the 1.1 V internal reference
  #if defined(__AVR_ATmega2560__)
     analogReference(INTERNAL1V1);
  #else
     analogReference(INTERNAL);
  #endif
}

void setup()  
{ 
  dht.setup(DHT_DATA_PIN); // set data pin of DHT sensor
  
  // Sleep for the time of the minimum sampling period to give the sensor time to power up
  // (otherwise, timeout errors might occure for the first reading)
  sleep(dht.getMinimumSamplingPeriod());
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Temp + Hum Sensor, Batt Pwr", "1.2");

  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);

  metric = getConfig().isMetric;
}

void loop()     
{     
    // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
   #ifdef MY_DEBUG
   Serial.print("Battery sense pin value: ");
   Serial.println(sensorValue);
   #endif

   // 1M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
   // 3.44/1023 = Volts per bit = 0.003363075

    // 1.5M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1.5e6+470e3)/470e3)*1.1 = Vmax =  4,610638297872340425531914893617 Volts
   // 4,610638297872340425531914893617 / 1023 = Volts per bit = 0,00450697780828185769846716998399
   
   int batteryPcnt = sensorValue / 10;

   #ifdef MY_DEBUG
   float batteryV  = sensorValue * 0.00450697780828185769846716998399;
   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
   #endif

   if (oldBatteryPcnt != batteryPcnt) {
     // Power up radio after sleep
     sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }

    dht.readSensor(true);
    // Fetch temperature from DHT sensor
    float temperature = dht.getTemperature();
    
    if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT!");
    } else {
      if (!metric) {
        temperature = dht.toFahrenheit(temperature);
      }
      
      // Reset no updates counter
      temperature += SENSOR_TEMP_OFFSET;

      #ifdef MY_DEBUG
      Serial.print("Value for DHT temp sensor: ");
      Serial.println(temperature);
      #endif
      
      // Only send temperature if it changed since the last measurement
      float diff = abs(lastTemp - temperature);
      if(diff > 0.01) {
        lastTemp = temperature;
        send(msgTemp.set(temperature, 1));
      }  
    }

    // Get humidity from DHT library
    float humidity = dht.getHumidity();
    if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
    } else {
      #ifdef MY_DEBUG
      Serial.print("H: ");
      Serial.println(humidity);
      #endif

      // Only send humidity if it changed since the last measurement
      float diff = abs(lastHum - humidity);
      if(diff > 0.01) {
        lastHum = humidity;
        send(msgHum.set(humidity, 1));
      }
    }

    // Send heartbeat
    if(millis() - lastHeartbeatTime > MAX_SLEEP_TIME)
    {
      sendHeartbeat();
      lastHeartbeatTime = millis();
    }
  
    sleep(MIN_SLEEP_TIME);
}
