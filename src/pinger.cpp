#include <Arduino.h>
#include <freertos/task.h>

#include "pinger.h"

#include <WiFi.h>
#include <ESP32Ping.h>

struct info {
  char *host;
  bool *pingSuccess;
} pingInfo = {0};

void pingTask(void *parms) {
  (void)parms;

  Serial.printf("Task '%s' running on core %u\n", pcTaskGetTaskName(NULL), xPortGetCoreID());

  for (;;) {
    *pingInfo.pingSuccess = Ping.ping(pingInfo.host, 2);
    delay(1000); // ping every second
  }
}

void startPing(const char *host, bool *pingSuccess) {
  uint32_t pingCpuId = 0;
  if (portNUM_PROCESSORS > 1 && xPortGetCoreID() == 0) {
    pingCpuId = 1; // use the other core...
  }
  pingInfo.host = strdup(host);
  pingInfo.pingSuccess = pingSuccess;
  xTaskCreatePinnedToCore(pingTask, "ping", 5 * 1024, nullptr, 0, nullptr, pingCpuId);
}
