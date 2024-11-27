#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
ESP8266WiFiMulti wifiMulti;
#endif
#ifdef ESP32
#include <Wifi.h>
#include <WifiMulti.h>
WiFiMulti wifiMulti;
#endif

#ifdef ESP8266
#include <ESPAsyncDNSServer.h>
AsyncDNSServer dnsServer;
#endif
const byte DNS_PORT = 53;
const uint32_t connectTimeoutMs = 10000;
const IPAddress ap_local_IP(4, 3, 2, 1);
const IPAddress ap_gateway(4, 3, 2, 1);
const IPAddress ap_subnet(255, 255, 255, 0);

#include <stdio.h>
String macToString(const unsigned char* mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}
bool blinkFlag;
#ifdef ESP8266
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  Serial.print("Station connected: ");
  Serial.println(macToString(evt.mac));
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
  Serial.print("Station disconnected: ");
  Serial.println(macToString(evt.mac));
}

void onProbeRequestPrint(const WiFiEventSoftAPModeProbeRequestReceived& evt) {
  Serial.print("Probe request from: ");
  Serial.print(macToString(evt.mac));
  Serial.print(" RSSI: ");
  Serial.println(evt.rssi);
}
void onProbeRequestBlink(const WiFiEventSoftAPModeProbeRequestReceived&) {
  // We can't use "delay" or other blocking functions in the event handler.
  // Therefore we set a flag here and then check it inside "loop" function.
  blinkFlag = true;
}
#endif



void start_ap() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet);
  WiFi.softAP(getAddDefault("AP_SSID",DEFAULT_AP_SSID), getAddDefault("AP_PSK",DEFAULT_AP_PSK));
  
  IPAddress myIP = WiFi.softAPIP();
  logdbg("AP IP address: "+myIP.toString());
  #ifdef ESP8266
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "*", myIP);
  #endif
  
  start_server();
}
#ifdef ESP8266
WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;
WiFiEventHandler probeRequestPrintHandler;
WiFiEventHandler probeRequestBlinkHandler;
#endif

int tries = 5;
void wifi_setup() {
  // Call "onStationDisconnected" each time a station disconnects
  // Call "onProbeRequestPrint" and "onProbeRequestBlink" each time
  // a probe request is received.
  // Former will print MAC address of the station and RSSI to Serial,
  // latter will blink an LED.
#ifdef ESP8266
  stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
  stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
  probeRequestPrintHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequestPrint);
#endif
  //probeRequestBlinkHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequestBlink);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  if (getAddDefault("WIFI_MODE", DEF_WIFI_MODE).equals("AP")) {
    start_ap();
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  int wificount = atoi(getAddDefault("WIFI_COUNT","1").c_str());
  getAddDefault("WIFI1_SSID",DEF_WIFI1_SSID);
  getAddDefault("WIFI1_PSK",DEF_WIFI1_PSK);
  
  for(int i =1;i<=wificount;i++){
    String ssid = getAddDefault("WIFI"+String(i)+"_SSID","");
    logdbg(String("loaded ")+ssid);
    wifiMulti.addAP(ssid.c_str(), getAddDefault("WIFI"+String(i)+"_PSK","").c_str());
  }
}
bool wificonndone = false;
long wifitimer = millis();
void wifi_loop(){
  if(!wificonndone){
    if(wifitimer+100<millis()){
      wifitimer = millis();
      logdbg("wifi multirun");
      if (wifiMulti.run() != WL_CONNECTED) {
        logdbg("Connection Failed! starting fallback ap in "+String(tries));
        if (tries <= 0) {
          start_ap();
          wificonndone=true;
        }
        tries--;
      }else{
        logdbg("WiFi connected: "+WiFi.SSID()+" IP: "+WiFi.localIP().toString());
        start_server();
        wificonndone=true;
      }
    }
  }
  if(shouldscan){
    logdbg("start scan");
    scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);  
    logdbg(String(scanResult)+"start done");
    int n = scanResult;
    if (n == WIFI_SCAN_FAILED)
    {
      logdbg(F("WIFI_SCAN_FAILED!"));
      events.send("scanfailed", "ota");
    }
    else if (n == WIFI_SCAN_RUNNING)
    {
      logdbg(F("WIFI_SCAN_RUNNING!"));
      events.send("scanrunning", "ota");
    }
    else if (n < 0)
    {
      logdbg(F("Failed, unknown error code!"));
      events.send("error", "ota");
    }
    else if (n == 0)
    {
      logdbg(F("No network found"));
      events.send("nonetworksfound", "ota");
      // page += F("No networks found. Refresh to scan again.");
    }else{
      events.send("done", "ota");  
    }
    shouldscan=false;
  }  
}
