# esp8266_lgawhp - HA integration for LG AWHP
HomeAssistant integration for older LG ThermaV (Therma V) 2010+ R410A/R32 Air/Water-Heat pumps.

# About
Based on the results from https://github.com/cribskip/esp8266_lgawhp/

# TODO
Maybe trying to emulate LCD panel as secondary slave device

# Hardware
## esp8266_lgawhp

![image](https://github.com/user-attachments/assets/b04c963c-6921-4ad8-88f0-d8e3aaffe302)

I do have some spare PCBs to sell in case you need one.

# Usage
Compile and upload to ESP8266 device - I used D1 Mini in my setup.
You will collect many data in HA, ex. below defrost timing:

![image](https://github.com/user-attachments/assets/0440ae6b-4022-4d41-a7ab-add342e1e14f)

# LG AWHP Protocol
## Glossary
Abbrev | Meaning
--- | ---
*AWHP* | Air/Water heat pump
*ODU* | Outdoor unit
*IDU* | Indor unit (monobloc includes this in the ODU), not to be confused with the REMO
*REMO* | Remote for control

## General
  - Single-wire half-duplex 300 Baud serial bus on the "Signal" line between ODU and REMO
  - Packets are 20 Bytes in size
  - Each (non-changing) packet is transmitted twice
  - Each packet has a **checksum** in the last byte

## Packet

### Status update / command
![image](https://github.com/user-attachments/assets/7ae5d7f3-4c17-40d3-b628-d3da0f155a73)


