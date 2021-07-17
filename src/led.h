#ifndef LED_H
#define LED_H

// Call this to start led task
// It will blink on start (white), wlan error (magenta) or influx error (cyan).
// It will also blink (green to red) if perfusion index is too low (100 to 10 or below)
// If F7 is connected it will calculate a solid health color from SpO2 and Ppm
void startLed(bool *wlanConnected, bool *pingOk, bool *echoSuccess );

#endif