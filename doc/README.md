# LMIC Documentation

This directory contains documentation on the use and implementation of the LMIC.

## Usage documentation

- [`LMIC-v3.0.99.pdf`](./LoRaWAN-v3.0.99.pdf): API documentation on the LMIC as of 3.0.99. Generally the differences between 3.0.99 and 3.2 were bug fixes.

## Background information

- [`LoRaWAN-at-a-glance.pdf`](./LoRaWAN-at-a-glance.pdf): a wall chart showing key features of the LoRaWAN 1.0.3 protocol.

## Implementation documentation

- [`RadioDriver.md`](./RadioDriver.md): documentation of the radio driver interface.
- [`LMIC-structure.pdf`](./LMIC-structure.pdf): a structural diagram of the LMIC. This is somewhat UML like.
- [`LMIC-FSM.pdf`](./LMIC-FSM.pdf): the operating logic of the LMIC, modeled as a finite state machine. As of version 3.2, this model is idealized; the actual implementation is not an explicit state machine. However, if you search for `os_setCallback()` and `os_setTimedCallback()`, you will see the links between event callbacks, and that will generally correspond to the implementation.  The FSM diagram doesn't show class-B or class-C operation as yet.

## Historical information

- `IBM-DISCLAIMER.txt` and `IBM-release-notes.txt` are artifacts of the original IBM distribution, retained for reference.

## Meta

Source files are included for documents that have separate sources.

- The source for the API documentation is a Microsoft Word file.
- The source for "LoRaWAN at a glance" is a Visio file.
- The sources for LMIC-FSM and LMIC-structure are [Cadifra](https://www.cadifra.com/) files. Cadifra is an inexpensive ($50) commercial tool that the author uses as a UML whiteboard; it's really lightweight and very cleanly implemented on Windows.
