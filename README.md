# esp8266_lgawhp - HA integration for LG AWHP
HomeAssistant integration for older LG ThermaV (ThermaV) 2010+ R410A/R32 Air/Water-Heat pumps.

# About
Based on the results from https://github.com/cribskip/esp8266_lgawhp/

# TODO
Maybe trying to emulate LCD panel as secondary slave device

# Hardware
## esp8266_lgawhp

![image](https://github.com/user-attachments/assets/b04c963c-6921-4ad8-88f0-d8e3aaffe302)

Let me know if you want to buy PCB as I have some spare ones.

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
00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19
--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ---
x0 | 22 | 02 | 08 | 00 | 19 | 00 | 00 | 14 | 2D | 00 | 17 | 11 | 26 | C0 | 00 | 06 | 40 | 00 | 2F
SRC | MODE1 | MODE2 | MODE3 | MODE4 | **UNK** | **UNK** | **UNK** | Water_target | DHW_target | **UNK** | Water_In | Water_Out | DHW | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | ChSum

