Arduino-LMIC proof-of-concept library
=====================================
This repository contains the IBM LMIC (LoraMAC-in-C) library, slightly
modified to run in the Arduino environment, allowing using the SX1272
LoRa tranceiver with an Arduino.

This repository is just a proof-of-concept to send data between two
SX1272 modules. It is not intended as a complete library and was not
tested in a full LoraWAN setup with a gateay (though feel free to fork
it and turn it into one if you want).

If you want to actually use this code, note that it contains version 1.4
of the library, but there are newer versions available already. Also,
the LMIC library is modified to allow device-to-device communication,
at the same time breaking device-to-gateway communication. If you need
that, make sure to revert commit 7561aa74c (Do not invert I/Q signals to
allow communication between nodes).

License
=======
The source files in this repository are made available under the Eclipse
Public License v1.0, except for the examples which use a more liberal
license. Refer to each individual source file for more details.

