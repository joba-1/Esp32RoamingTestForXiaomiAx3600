#include <Arduino.h>
#include <freertos/task.h>

#include "led.h"

// Show wlan and other status via rgb led
#include <Adafruit_NeoPixel.h>

static uint8_t dimming_factor = 2;
static uint8_t maxRed = 240 / dimming_factor;
static uint8_t maxGreen = 255 / dimming_factor;
static uint8_t maxBlue = 255 / dimming_factor;

static const uint32_t neutralColor = Adafruit_NeoPixel::Color(0, 0, 0);
static const uint32_t startColor = Adafruit_NeoPixel::Color(maxRed, maxGreen, maxBlue);
static const uint32_t wlanErrorColor = Adafruit_NeoPixel::Color(maxRed, 0, 0);
static const uint32_t pingErrorColor = Adafruit_NeoPixel::Color(0, 0, maxBlue);
static const uint32_t echoErrorColor = Adafruit_NeoPixel::Color(maxRed, 0, maxBlue);
static const uint32_t okColor = Adafruit_NeoPixel::Color(0, maxGreen, 0);

static const uint8_t neopixelPin = 27; // m5 atom lite
static Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, neopixelPin, NEO_GRB + NEO_KHZ800);

static volatile uint32_t buttonPressed = 0;

const char *lastError = "OK";

struct status {
  bool *wlanConnected;
  bool *pingSuccess;
  bool *echoSuccess;
} networkStatus = {0};

// different colors for no connect, no ping and ping ok
uint32_t calculateColor() {
  if (!*networkStatus.wlanConnected) {
    lastError = "No WLAN";
    return wlanErrorColor;
  }

  if (!*networkStatus.pingSuccess) {
    lastError = "No Ping";
    return pingErrorColor;
  }

  if (!*networkStatus.echoSuccess) {
    lastError = "No Echo";
    return echoErrorColor;
  }

  lastError = "OK";
  return okColor;
}

void ledTask( void *parms ) {
  (void)parms;

  Serial.printf("Task '%s' running on core %u\n", pcTaskGetTaskName(NULL), xPortGetCoreID());

  static uint32_t lastColor = !startColor;
  static bool useLed = true;

  pixel.begin(); // This initializes the NeoPixel library.
  pixel.setPixelColor(0, startColor);
  pixel.show();

  pinMode(39, INPUT_PULLUP); // m5 atom lite

  for (;;) {
    if( digitalRead(39) == LOW ) {
      buttonPressed++;
    } else {
      buttonPressed = 0;
    }

    if( buttonPressed == 20 ) {
      useLed = !useLed;
      if( useLed ) {
        Serial.println("LED on");
      } else {
        Serial.println("LED off");
        pixel.setPixelColor(0, neutralColor);
        lastColor = 0;
        pixel.show();
      }
    }

    if( useLed ) {
      uint32_t newColor = calculateColor();
      if( newColor != lastColor ) {
        lastColor = newColor;
        pixel.setPixelColor(0, newColor);
        pixel.show();
      }
    }
    // We run in lowest priority, no need for a delay()...
  }
}

void startLed(bool *wlanConnected, bool *pingSuccess, bool *echoSuccess) {
  uint32_t ledCpuId = 0;
  if (portNUM_PROCESSORS > 1 && xPortGetCoreID() == 0) {
    ledCpuId = (portNUM_PROCESSORS > 2) ? 2 : 1; // use another core...
  }
  networkStatus.wlanConnected = wlanConnected;
  networkStatus.pingSuccess = pingSuccess;
  networkStatus.echoSuccess = echoSuccess;
  xTaskCreatePinnedToCore(ledTask, "led", 5 * 1024, nullptr, 0, nullptr, ledCpuId);
}
