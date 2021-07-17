#include <Arduino.h>
#include <freertos/task.h>

#include "echo.h"

#include <WiFi.h>

struct echo {
  char *host;
  uint16_t port;
  bool *success;
  char *status;
} echo = {0};

void echoTask(void *parms) {
  (void)parms;

  Serial.printf("Task '%s' running on core %u\n", pcTaskGetTaskName(NULL), xPortGetCoreID());

  WiFiClient client;
  uint8_t byte = 'a';

  for (;;) {
    if (!client.connected()) {
      strcpy(echo.status, "Echo disconnect");
      client.connect(echo.host, echo.port);
    }
    if (client.connected() && client.write(byte) == sizeof(byte)) {
      // client.flush();
      uint32_t start = millis();
      uint32_t end = start;
      int echoByte = -1;
      while (end - start < ECHO_TIMEOUT_MS) {
        echoByte = client.read();
        if (echoByte != -1) {
          break;
        }
        end = millis();
      }
      *echo.success = (echoByte == byte);
      if( echoByte == -1 ) {
        strcpy(echo.status, "Echo timeout");
      }
      else if( !*echo.success ) {
        strcpy(echo.status, "Echo read error");
      }
    }
    else {
      strcpy(echo.status, "Echo write error");
      *echo.success = false;
    }

    // always sending the same stuff is boring...
    byte = (byte == 'z') ? 'a' : byte + 1; 

    delay(1000); // send every second
  }
}

void startEcho(const char *host, uint16_t port, bool *echoSuccess, char *status) {
  uint32_t echoCpuId = 0;
  if (portNUM_PROCESSORS > 1 && xPortGetCoreID() == 0) {
    echoCpuId = 1; // use the other core...
  }
  echo.host = strdup(host);
  echo.port = port;
  echo.success = echoSuccess;
  echo.status = status;
  xTaskCreatePinnedToCore(echoTask, "echo", 5 * 1024, nullptr, 0, nullptr, echoCpuId);
}
