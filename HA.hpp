#include "IPAddress.h"

#include <WiFi.h>
#include <ArduinoHA.h>

#include "config.h"


WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

HASensorNumber ha_voltage("voltage", HASensorNumber::PrecisionP2);
HASensorNumber ha_amperage("amperage", HASensorNumber::PrecisionP2);
HASensorNumber ha_power("power", HASensorNumber::PrecisionP2);
HASensorNumber ha_powerfactor("powerfactor", HASensorNumber::PrecisionP2);
HASensorNumber ha_freqency("freqency", HASensorNumber::PrecisionP2);
HASwitch ha_switch("switch");

void setupWifi(){
  
  byte mac[6];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  device.setName("ESP32_SEM6000_GateWay");
  device.setSoftwareVersion("1.0.0");

  ha_switch.setIcon("mdi:lightbulb");
  ha_switch.setName("My LED");
  //led.onCommand(onSwitchCommand);
    
  ha_voltage.setName("Voltage");
  ha_voltage.setUnitOfMeasurement("V");

  ha_amperage.setName("Amperage");
  ha_amperage.setUnitOfMeasurement("A");

  ha_power.setName("Power");
  ha_power.setUnitOfMeasurement("W");

  ha_powerfactor.setName("Power Factor");
  ha_powerfactor.setUnitOfMeasurement("#");

  ha_freqency.setName("Frequency");
  ha_freqency.setUnitOfMeasurement("Hz");
  
  mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
}
long nexttime = 0;
bool onconnect = true;
//IPAddress BROKER_ADDR=IPAddress();
int tries=10;
void wifiloop() {
  mqtt.loop();

  if (WiFi.status() == WL_CONNECTED){
    if(onconnect){
      onconnect=false;
      //WiFi.hostByName("homeassistant.fritz.box",BROKER_ADDR);
      //Serial.println(BROKER_ADDR.toString());
      mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
      //mqtt.begin("10.0.0.16",1883,BROKER_USERNAME,BROKER_PASSWORD);
      
    }
  }else{
    delay(1000);
    Serial.print(".");
  }
  long now = millis();
  if(nexttime<now){
    nexttime = now+5000;
    if(mqtt.isConnected()){
      //Serial.println("setValue");
    }else{
      Serial.println("mqtt not connected");
      //WiFi.hostByName("homeassistant.fritz.box",BROKER_ADDR);
      //Serial.println(BROKER_ADDR.toString());
      mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
    }
  }
  
}