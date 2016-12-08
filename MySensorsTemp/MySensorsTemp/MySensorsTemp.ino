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
#include <DallasTemperature.h>
#include <OneWire.h>

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

#define ONE_WIRE_BUS 3 // Pin where dallas sensor is connected 
#define MAX_ATTACHED_DS18B20 16

unsigned long MIN_SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
unsigned long MAX_SLEEP_TIME = 3600000; //Maximum sleep time, after this it will send a keepAlive message

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 

float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
bool receivedConfig = false;
bool metric = true;

uint8_t lastHeartbeatTime = 0;

// Initialize temperature message
MyMessage msg(0,V_TEMP);

void before()
{
  // use the 1.1 V internal reference
  #if defined(__AVR_ATmega2560__)
     analogReference(INTERNAL1V1);
  #else
     analogReference(INTERNAL);
  #endif
  
  // Startup up the OneWire library
  sensors.begin();
}

void setup()  
{ 
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Temp Sensor, Batt Pwr", "1.2");

  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i, S_TEMP);
  }
}

void loop()     
{     
    // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
   #ifdef MY_DEBUG
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
  
    // Fetch temperatures from Dallas sensors
    sensors.requestTemperatures();
  
    // query conversion time and sleep until conversion completed
    int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
    // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
    sleep(conversionTime);
  
    // Read temperatures and send them to controller 
    for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
   
      // Fetch and round temperature to one decimal
      float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
   
      // Only send data if temperature has changed and no error
      if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) { 
        // Send in the new temperature
        send(msg.setSensor(i).set(temperature,1));
        sendHeartbeat();
        // Save new temperatures for next compare
        lastTemperature[i]=temperature;
      }
    }

    if(millis() - lastHeartbeatTime > MAX_SLEEP_TIME)
    {
      sendHeartbeat();
      lastHeartbeatTime = millis();
    }
  
    sleep(MIN_SLEEP_TIME);
}
