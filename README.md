# WLAN Roaming Test

the WIP openwrt firmware of the Xiaomi AX3600 router by robimarko seems to have problems with roaming clients between two LAN connected AP devices.

There is always a delay of about 1-4 minutes until the client device can communicate again. Happens with Notebooks, iPhones, Android phones ... and now with my ESP32.

This ESP32 firmware now changes the MAC on restarts and it restarts on low RSSI or connection loss.
I also tested it with static ip (to keep the IP the same over reconnects). Same result:

Changing the MAC results in immediate reconnects on every roam (although it isn't really a roam but more like a new device connect). 
This should prove it is not a WLAN issue (4-way-handshake happens) and not an IP issue (routing, ping, tcp send/receive all works after some time).
DHCP also works up until the ACK from the client. Only then communication stalls.

Curent hypothesis: some switch port issue (hardware and software port get out of sync)

Will investigate with brctl and bridge commands to see what's going on on this level - without changing the MAC.

* https://openwrt.org/inbox/toh/xiaomi/xiaomi_ax3600
* https://forum.openwrt.org/t/adding-openwrt-support-for-xiaomi-ax3600/55049
* https://forum.openwrt.org/t/roaming-issues-xiaomi-ax3600/101333/15
* https://github.com/robimarko/openwrt/tree/AX3600-5.10-restart
