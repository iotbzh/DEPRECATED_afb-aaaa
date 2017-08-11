
# HAL for MOST Sound Card

## Introduction
This Hardware Abstraction Layer (HAL) is intended to control MOST audio via
standard ALSA mixer controls. Therefore the HAL add's controls to the MOST sound
card. Modification of the mixer controls is mapped to control commands which are
send to MOST nodes.

The HAL requires to access the UNICENS V2 binding which is setting up the MOST
network and the MOST node.

Please check the following required components:
* MOST Linux Driver
* User must be member of "audio" group
* UNICENS V2 binding
* [K2L MOST150 Audio 5.1 Kit](https://www.k2l.de/products/74/MOST150%20Audio%205.1%20Kit/)

## Possible Modifications
Check if you need to adapt the default path for UNICENS configuration, e.g.
```
#define XML_CONFIG_PATH "/opt/AGL/unicens2-binding/config_multichannel_audio_kit.xml"
```

Check if you need to adapt the name of the MOST sound card.
```
#define ALSA_CARD_NAME  "Microchip MOST:1"
```

You can check your MOST sound card name by calling ```aplay -l```, e.g.
```
aplay -l
**** List of PLAYBACK Hardware Devices ****
card 0: PCH [HDA Intel PCH], device 0: 92HD90BXX Analog [92HD90BXX Analog]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: ep016ch [Microchip MOST:1], device 0: ep01-6ch []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 2: ep022ch [Microchip MOST:2], device 0: ep02-2ch []
  Subdevices: 1/1
  Subdevice #0: subdevice #0

```
Choose the first sound card with 6 channels, e.g. if you see ```ep01-6ch``` just 
take ```Microchip MOST:1```.

If you get messed up with card enumeration the following action may help:
- Unplug your MOST USB hardware from target
- Call ```sudo rm /var/lib/alsa/asound.state```
- Connect MOST USB hardware after reboot or restart of ALSA
