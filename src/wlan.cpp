#include <Arduino.h>
#include <freertos/task.h>

#include "wlan.h"

#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <WlanConfig.h>

// for changing MAC
#include <esp_wifi.h>

const uint32_t MAGIC = 0x55aa1234;
RTC_NOINIT_ATTR uint8_t rtcMac[6];
RTC_NOINIT_ATTR uint32_t rtcMagic;

// Static IP address for changing MAC
IPAddress staticIp(192, 168, 1, 98);
IPAddress gateway(192, 168, 1, 221);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDns(192, 168, 1, 236);

// hostname pattern
static const char hostFormat[] = "%s-%s";

// NTP server parameters
static const char ntpServer[] = "de.pool.ntp.org";
static const long gmtOffset_sec = 3600;
static const int daylightOffset_sec = 3600;

// webserver and updater
static WebServer httpServer(80);
static HTTPUpdateServer httpUpdater;

static const char PAGE[] =
    "<!DOCTYPE html>\n"
    "<html>\n"
    " <head>\n"
    "  <title id='title'>" NAME " " VERSION "</title>\n"
    " </head>\n"
    " <body>\n"
    "  <h1>" NAME " " VERSION "</h1>\n"
    "  <button type='button' onclick='window.location.href=\"update\"'>Firmware Update</button>\n"
    "  <button type='button' onclick='window.location.href=\"reset\"'>Reset " NAME "</button>\n"
    " </body>\n"
    "</html>\n";

void handleRoot() {
  httpServer.send(200, "text/html", PAGE); 
}

void handleReset() {
  httpServer.send(200, "text/html", "<META http-equiv=\"refresh\" content=\"5;URL=/\">Rebooting...");
  delay(500);
  ESP.restart();
}

void handleNotFound() { 
  httpServer.send(404, "text/html", PAGE); 
}

void setHostname() {
  char hostName[100];
  String id = WiFi.macAddress().substring(9);
  id.remove(5, 1);
  id.remove(2, 1);
  snprintf(hostName, sizeof(hostName), hostFormat, NAME, id.c_str());
  WiFi.setHostname(hostName);
}

void printMac(const char *label, const uint8_t *mac) {
  Serial.printf("%s: %02x:%02x:%02x:%02x:%02x:%02x\n", 
    label, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void wlanTask(void *parms) {
  bool *wlanConnected = (bool *)parms;

  Serial.printf("Task '%s' running on core %u\n", pcTaskGetTaskName(NULL), xPortGetCoreID());

  wifi_init_config_t initCfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&initCfg);

  if (rtcMagic != MAGIC) {
    esp_wifi_get_mac(WIFI_IF_STA, rtcMac);
    rtcMagic = MAGIC;
    printMac("init magic", rtcMac);
  }
  else {
    rtcMac[5]++;
    printMac("found magic", rtcMac);
  }

  if (esp_wifi_set_mac(WIFI_IF_STA, rtcMac) != ESP_OK) {
    rtcMac[5]++;
    printMac("retry magic", rtcMac);
    esp_wifi_set_mac(WIFI_IF_STA, rtcMac);
  }

    setHostname();
    WiFi.config(staticIp, gateway, subnet, primaryDns);

    do {
      WiFi.begin(WlanConfig::Ssid, WlanConfig::Password);
  } while (WiFi.waitForConnectResult() != WL_CONNECTED);

  configTime(gmtOffset_sec, daylightOffset_sec, WiFi.gatewayIP().toString().c_str(), ntpServer);

  *wlanConnected = true;
  
  MDNS.begin(WiFi.getHostname());
  httpUpdater.setup(&httpServer);
  httpServer.on("/", handleRoot);
  httpServer.on("/reset", handleReset);
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);

  for (;;) {
    *wlanConnected = WiFi.isConnected();
    httpServer.handleClient();
    // We run in lowest priority, no need for a delay()...
  }
}

void startWlan( bool *wlanConnected ) {
  uint32_t wlanCpuId = 0;
  if (portNUM_PROCESSORS > 1 && xPortGetCoreID() == 0) {
    wlanCpuId = 1; // use the other core...
  }
  xTaskCreatePinnedToCore(wlanTask, "wlan", 5 * 1024, wlanConnected, 0, nullptr, wlanCpuId);
}
