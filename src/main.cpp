/*
WLAN Roaming Test

* connect to a wlan
* log connects and disconnects
* log start ping success and start ping failure
* show local status with RGB LED
  * RED: disconnected
  * GREEN: connected and ping OK
  * BLUE: connected but no ping

Algorithm
=========

connect to WLAN
forever
  if status connected
    if was disconnected
      if first connect
        get time
      else
        syslog disconnect time
      syslog new online time
    if ping succeeds
      if was failed 
          syslog failed time
          syslog current success time
    else
      if not remembered faile
          remember failed time
  else
    if not remembered disconnect
      remember disconnect time
*/

#include <Arduino.h>

#include <WiFi.h>
#include <Syslog.h>
#include <WiFiUdp.h>

#include "wlan.h"
#include "led.h"
#include "pinger.h"
#include "echo.h"

#include <time.h>

// Syslog server
static const char syslogServer[] = "job4";
static const int syslogPort = 514;
static WiFiUDP syslogUdp;
static Syslog syslog(syslogUdp, SYSLOG_PROTO_IETF);

struct tm disconnect_time = {0};
struct tm no_ping_time = {0};
struct tm no_echo_time = {0};

bool wlanConnected = false;
bool pingSuccess = false;
bool echoSuccess = false;

char echoStatus[80];

void log(const char *what, struct tm *when) {
  char when_str[80];
  strftime(when_str, sizeof(when_str), "%Y-%m-%d %T", when);
  char msg[80];
  snprintf(msg, sizeof(msg), "%s at %s\n", what, when_str);
  Serial.print(msg);
  syslog.log(LOG_NOTICE, msg);
}

void stayConnected(bool wlanConnected) {
  static uint32_t disconnected_since = 0;
  static uint32_t reconnecting_since = 0;

  if (wlanConnected) {
    disconnected_since = 0;
    reconnecting_since = 0;
  } else {
    uint32_t now = millis() | 1;
    
    if (disconnected_since == 0) {
      disconnected_since = now;
    }
    else if ((now - disconnected_since) > 1000 * 60) {
      struct tm tmNow;
      getLocalTime(&tmNow);
      log("Restart", &tmNow); // serial only...
      delay(500);
      ESP.restart();
    }
    
    if ((reconnecting_since == 0) || ((now - reconnecting_since) > 1000 * 10)) {
      struct tm tmNow;
      getLocalTime(&tmNow);
      log("Reconnect", &tmNow); // serial only...
      WiFi.reconnect();
      reconnecting_since = now;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n" NAME " " VERSION);
  Serial.println("Compiled " __DATE__ " " __TIME__);
  Serial.printf("Task '%s' running on core %u\n", pcTaskGetTaskName(NULL), xPortGetCoreID());

  startLed(&wlanConnected, &pingSuccess, &echoSuccess);
  startWlan(&wlanConnected);
  startPing("job4", &pingSuccess);
  startEcho("job4", 7, &echoSuccess, echoStatus);
}

void loop() {
  static bool wasDisconnected = true;
  static bool wasNoPing = true;
  static bool wasNoEcho = true;
  static bool firstConnect = true;
  static long rssi = 0;

  stayConnected(wlanConnected);

  if (wlanConnected) {
    if (wasDisconnected) {
      wasDisconnected = false;
      if (firstConnect) {
        firstConnect = false;
        // Syslog setup
        syslog.server(syslogServer, syslogPort);
        syslog.deviceHostname(WiFi.getHostname());
        syslog.appName(NAME);
        syslog.defaultPriority(LOG_KERN);

        char msg[80];
        snprintf(msg, sizeof(msg), "Host %s %s started with IP %s\n",
                 WiFi.getHostname(), VERSION, WiFi.localIP().toString().c_str());
        Serial.print(msg);
        syslog.log(LOG_NOTICE, msg);
      }
      else {
        log("Disconnect", &disconnect_time);
      }
      struct tm now;
      getLocalTime(&now);
      log("Connect", &now);
      log(WiFi.macAddress().c_str(), &disconnect_time);
    }

    long newRssi = WiFi.RSSI();
    if( abs(newRssi - rssi) >= 5 ) {
      rssi = newRssi;
      char msg[80];
      snprintf(msg, sizeof(msg), "RSSI -> %ld", rssi);
      struct tm now;
      getLocalTime(&now);
      log(msg, &now);
    }

    { 
      static unsigned int lowRssiCount = 0;
      if (rssi < -85) {
        if (++lowRssiCount > 100) {
          struct tm now;
          getLocalTime(&now);
          log("Reconnect low RSSI", &now);
          delay(500);
          WiFi.reconnect();

          // log("Restart low RSSI", &now);
          // delay(500);
          // ESP.restart();
        }
      } else {
        lowRssiCount = 0;
      }
    }

    if (pingSuccess) {
      if (wasNoPing) {
        wasNoPing = false;
        log("No ping", &no_ping_time);
        struct tm now;
        getLocalTime(&now);
        log("Ping ok", &now);
      }
    } else {
      if (!wasNoPing) {
        wasNoPing = true;
        getLocalTime(&no_ping_time);
      }
    }

    if (echoSuccess) {
      if (wasNoEcho) {
        wasNoEcho = false;
        log(echoStatus, &no_echo_time);
        struct tm now;
        getLocalTime(&now);
        log("Echo ok", &now);
      }
    } else {
      if (!wasNoEcho) {
        wasNoEcho = true;
        getLocalTime(&no_echo_time);
      }
    }

  } else {
    if (!wasDisconnected) {
      wasDisconnected = true;
      getLocalTime(&disconnect_time);
    }
  }

  delay(10);
}
