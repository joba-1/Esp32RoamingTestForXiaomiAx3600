# WLAN Roaming Test

the WIP openwrt firmware of the Xiaomi AX3600 router by robimarko seems to have problems with roaming between two LAN connected devices.

There is always a delay of about 1-5 minutes until the device can communicate again.

This ESP32 firmware changes the MAC on restarts and it restarts on low RSSI or connection loss.

This results in immediate reconnects.

