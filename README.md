Arduino-LMIC proof-of-concept library
=====================================
This repository contains the IBM LMIC (LoraMAC-in-C) library, slightly
modified to run in the Arduino environment, allowing using the HopeRF 
RFM92/95 LoRa tranceiver with an Arduino Teensy (3.1 or LC).

This repository is just a proof-of-concept. 

If you want to actually use this code, note that it contains version 1.4
of the library, but there are newer versions available already. 

Connections
-----------
Note that the RFM92/95 module runs at 3.3V and likely does not like 5V on
its pins, so make sure to use a level shifter, or an Arduino running at
3.3V.

The pins to use are shown (and can be changed) in the pinmap in example
.ino files. Connecting RST is needed.

License
-------
The source files in this repository are made available under the Eclipse
Public License v1.0, except for the examples which use a more liberal
license. Refer to each individual source file for more details.

