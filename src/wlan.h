#ifndef WLAN_H
#define WLAN_H

// Call this to start wlan task
// It will connect to preconfigured wlan and get current time
// It will also start a webserver for firmware updates
void startWlan( bool *wlanConnected );

#endif