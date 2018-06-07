Arduino-LMIC library
====================

This repository contains the IBM LMIC (LoraMAC-in-C) library, slightly
modified to run in the Arduino environment, allowing using the SX1272,
SX1276 transceivers and compatible modules (such as some HopeRF RFM9x
modules and the Murata LoRa modules).

This library mostly exposes the functions defined by LMIC, it makes no
attempt to wrap them in a higher level API that is more in the Arduino
style. To find out how to use the library itself, see the examples, or
see the PDF file in the doc subdirectory.

The [MCCI arduino-lorawan](https://github.com/mcci-catena/arduino-lorawan) library provides a higher level, more Arduino-like wrapper which may be useful.

This library requires Arduino IDE version 1.6.6 or above, since it
requires C99 mode to be enabled by default.

[![GitHub release](https://img.shields.io/github/release/mcci-catena/arduino-lmic.svg)](https://github.com/mcci-catena/arduino-lmic/releases/latest) [![GitHub commits](https://img.shields.io/github/commits-since/mcci-catena/arduino-lmic/latest.svg)](https://github.com/mcci-catena/arduino-lmic/compare/V2.1.5...master) [![Build Status](https://travis-ci.org/mcci-catena/arduino-lmic.svg?branch=master)](https://travis-ci.org/mcci-catena/arduino-lmic)

**Contents:**

<!--
  This TOC uses the VS Code markdown TOC extension AlanWalk.markdown-toc.
  We strongly recommend updating using VS Code, the markdown-toc extension and the
  bierner.markdown-preview-github-styles extension.
-->
<!-- TOC depthFrom:1 -->

- [Installing](#installing)
- [Features](#features)
- [Configuration](#configuration)
	- [Selecting the LoRaWAN Region Configuration](#selecting-the-lorawan-region-configuration)
		- [eu868, as923, in866](#eu868-as923-in866)
		- [us915, au921](#us915-au921)
	- [Selecting the target radio transceiver](#selecting-the-target-radio-transceiver)
	- [Controlling use of interrupts](#controlling-use-of-interrupts)
	- [Disabling PING](#disabling-ping)
	- [Disabling Beacons](#disabling-beacons)
	- [Rarely changed variables](#rarely-changed-variables)
		- [Changing debug output](#changing-debug-output)
		- [Getting debug from the RF library](#getting-debug-from-the-rf-library)
		- [Selecting the AES library](#selecting-the-aes-library)
		- [Defining the OS Tick Frequency](#defining-the-os-tick-frequency)
		- [Setting the SPI-bus frequency](#setting-the-spi-bus-frequency)
		- [Changing handling of runtime assertion failures](#changing-handling-of-runtime-assertion-failures)
		- [Disabling JOIN](#disabling-join)
		- [Disabling Class A MAC commands](#disabling-class-a-mac-commands)
		- [Disabling Class B MAC commands](#disabling-class-b-mac-commands)
		- [Special purpose](#special-purpose)
- [Supported hardware](#supported-hardware)
- [Connections](#connections)
	- [Power](#power)
	- [SPI](#spi)
	- [DIO pins](#dio-pins)
	- [Reset](#reset)
	- [RXTX](#rxtx)
	- [RXTX Polarity](#rxtx-polarity)
	- [Pin mapping](#pin-mapping)
		- [Adafruit Feather M0 LoRa](#adafruit-feather-m0-lora)
		- [LoRa Nexus by Ideetron](#lora-nexus-by-ideetron)
		- [MCCI Catena 4450/4460](#mcci-catena-44504460)
		- [MCCI Catena 4551](#mcci-catena-4551)
- [Example Sketches](#example-sketches)
- [Timing](#timing)
- [Downlink datarate](#downlink-datarate)
- [Release History](#release-history)
- [Contributions](#contributions)
- [Trademark Acknowledgements](#trademark-acknowledgements)
- [License](#license)

<!-- /TOC -->

## Installing

To install this library:

 - install it using the Arduino Library manager ("Sketch" -> "Include
   Library" -> "Manage Libraries..."), or
 - download a zipfile from github using the "Download ZIP" button and
   install it using the IDE ("Sketch" -> "Include Library" -> "Add .ZIP
   Library..."
 - clone this git repository into your sketchbook/libraries folder.

For more info, see https://www.arduino.cc/en/Guide/Libraries

## Features

The LMIC library provides a fairly complete LoRaWAN Class A and Class B
implementation, supporting the EU-868, US-915, AU-921, AS-923, and IN-866 bands. Only a limited
number of features was tested using this port on Arduino hardware, so be careful when using any of the untested features.

The library has only been tested with LoRaWAN 1.0.2 networks and does not have the separated key structure defined by LoRaWAN 1.1.

What certainly works:

 - Sending packets uplink, taking into account duty cycling.
 - Encryption and message integrity checking.
 - Receiving downlink packets in the RX2 window.
 - Custom frequencies and datarate settings.
 - Over-the-air activation (OTAA / joining).
 - Receiving downlink packets in the RX1 and RX2 windows.
 - Some MAC command processing.

What has not been tested:

 - Receiving and processing all MAC commands.
 - Class B operation.
 - FSK has not been extensively tested.

If you try one of these untested features and it works, be sure to let
us know (creating a github issue is probably the best way for that).

## Configuration

A number of features can be enabled or disabled at compile time.
This is done by adding the desired settings to the file
`project_settings/lmic_project_config.h`. The `project_settings`
directory is the only directory that contains files that you
should edit to match your project; we organize things this way
so that your local changes are more clearly separated from
the distribution files. The Arduino environment doesn't give
us a better way to do this.

Unlike other ports of the LMIC code, in this port, you should not edit `src/lmic/config.h` to configure this package.

The following configuration variables are available.

### Selecting the LoRaWAN Region Configuration

The library supports the following regions:

`-D` variable | CFG region name | CFG region value | LoRa Spec Reference| Frequency
------------|-----------------|:----------------:|:-------------------:|--------
`-D CFG_eu868` | `LMIC_REGION_eu868` | 1 | 2.1 | EU 863-870 MHz ISM
`-D CFG_us915` | `LMIC_REGION_us915` | 2 | 2.2 | US 902-928 MHz ISM
`-D CFG_au921` | `LMIC_REGION_au921` | 5 | 2.5 | Australia 915-928 MHz ISM
`-D CFG_as923` | `LMIC_REGION_as923` | 7 | 2.7 | Asia 923 MHz ISM
`-D CFG_in866` | `LMIC_REGION_in866` | 9 | 2.9 | India 865-867 MHz ISM

You should define exactly one of `CFG_...` variables. If you don't,
the library assumes `CFG_eu868`. The library changes configuration pretty substantially
according to the region. Some of the differences are listed below.

#### eu868, as923, in866

If the library is configured for EU868, AS923, or IN866 operation, we make
the following changes:
- Add the API `LMIC_setupBand()`.
- Add the constants `MAX_CHANNELS`, `MAX_BANDS`, `LIMIT_CHANNELS`, `BAND_MILLI`,
`BAND_CENTI`, `BAND_DECI`, and `BAND_AUX`.

#### us915, au921

If the library is configured for US915 operation, we make the following changes:
- Add the APIs `LMIC_enableChannel()`,
`LMIC_enableSubBand()`, `LMIC_disableSubBand()`, and `LMIC_selectSubBand()`.
- Add the constants `MAX_XCHANNELS`.
- Add a number of additional `DR_...` symbols.

### Selecting the target radio transceiver

You should define one of the following variables. If you don't, the library assumes
sx1276. There is a runtime check to make sure the actual transceiver matches the library
configuration.

`#define CFG_sx1272_radio 1`

Configures the library for use with an sx1272 transceiver.

`#define CFG_sx1276_radio 1`

Configures the library for use with an sx1276 transceiver.

### Controlling use of interrupts

`#define LMIC_USE_INTERRUPTS`

If defined, configures the library to use interrupts for detecting events from the transceiver. If left undefined, the library will poll for events from the transceiver.  `LMIC_USE_INTERRUPTS` is not currently tested.

### Disabling PING

`#define DISABLE_PING`

If defined, removes all code needed for PING.  Removes the APIs `LMIC_setPingable()` and `LMIC_stopPingable()`.
Class A devices don't support PING, so defining `DISABLE_PING` is often a good idea.

By default, PING support is included in the library.

### Disabling Beacons

`#define DISABLE_BEACONS`

If defined, removes all code needed for handling beacons. Removes the APIs `LMIC_enableTracking()` and `LMIC_disableTracking()`.
Class A devices don't support beacons, so defining `DISABLE_BEACONS` might be a good idea.


### Rarely changed variables ###

The remaining variables are rarely used, but we list them here for completeness.

#### Changing debug output

`#define LMIC_PRINTF_TO SerialLikeObject`

This variable should be set to the name of a `Serial`-like object, used for printing messages. If not defined, `Serial`
is assumed.

#### Getting debug from the RF library

`#define LMIC_DEBUG_LEVEL number /* 0, 1, or 2 */`

This variable determines the amount of debug output to be produced by the library. The default is `0`.

If `LMIC_DEBUG_LEVEL` is zero, no output is produced. If `1`, limited output is produced. If `2`, more extensive
output is produced.  If non-zero, printf() is used, and the Arduino environment must be configured to support it,
otherwise the sketch will crash at runtime.

#### Selecting the AES library

The library comes with two AES implementations. The original implementation is better on
ARM processors becasue it's faster, but it's larger. For smaller AVR8 processors, a
second library ("IDEETRON") is provided that has a smaller code footprint.
You may define one of the following variables to choose the AES implementation. If you don't,
the library uses the IDEETRON version.

`#define USE_ORIGINAL_AES`

If defined, the original AES implementation is used.

`#define USE_IDEETRON_AES`

If defined, the IDEETRON AES implementation is used.

#### Defining the OS Tick Frequency

`#define US_PER_OSTICK_EXPONENT number`

This variable should be set to the base-2 logarithm of the number of microseconds per OS tick. The default is 4,
which indicates that each tick corresponds to 16 microseconds (because 16 == 2^4).

#### Setting the SPI-bus frequency

`#define LMIC_SPI_FREQ floatNumber`

This variable sets the default frequency for the SPI bus connection to the transceiver. The default is `1E6`, meaning 1 MHz. However, this can be overridden by the contents of the `lmic_pinmap` structure, and we recommend that you use that approach rather than editing the `project_settings/lmic_project_config.h` file.

####  Changing handling of runtime assertion failures

The variables `LMIC_FAILURE_TO` and `DISABLE_LMIC_FAILURE_TO`
control the handling of runtime assertion failures. By default, assertion messages are displayed using
the `Serial` object. You can define LMIC_FAILURE_TO to be the name of some other `Print`-like obect. You can
also define `DISABLE_LMIC_FAILURE_TO` to any value, in which case assert failures will silently halt execution.

#### Disabling JOIN

`#define DISABLE_JOIN`

If defined, removes code needed for OTAA activation. Removes the APIs `LMIC_startJoining()` and `LMIC_tryRejoin()`.

#### Disabling Class A MAC commands

`DISABLE_MCMD_DCAP_REQ`, `DISABLE_MCMD_DN2P_SET`, and `DISABLE_MCMD_SNCH_REQ` respectively disable code for various Class A MAC
commands.

#### Disabling Class B MAC commands

`DISABLE_MCMD_PING_SET` disables the PING_SET MAC commands. It's implied by `DISABLE_PING`.

`DISABLE_MCMD_BCNI_ANS` disables the next-beacon start command. I'ts implied by `DISABLE_BEACON'

#### Special purpose

`#define DISABLE_INVERT_IQ_ON_RX` disables the inverted Q-I polarity on RX. If this is defined, end-devices will be able
to receive messages from each other, but will not be able to hear the gateway.

## Supported hardware

This library is intended to be used with plain LoRa transceivers,
connecting to them using SPI. In particular, the SX1272 and SX1276
families are supported (which should include SX1273, SX1277, SX1278 and
SX1279 which only differ in the available frequencies, bandwidths and
spreading factors). It has been tested with both SX1272 and SX1276
chips, using the Semtech SX1272 evaluation board and the HopeRF RFM92
and RFM95 boards (which supposedly contain an SX1272 and SX1276 chip
respectively).

This library contains a full LoRaWAN stack and is intended to drive
these Transceivers directly. It is *not* intended to be used with
full-stack devices like the Microchip RN2483 and the Embit LR1272E.
These contain a transceiver and microcontroller that implements the
LoRaWAN stack and exposes a high-level serial interface instead of the
low-level SPI transceiver interface.

This library is intended to be used inside the Arduino environment. It
should be architecture-independent, so it should run on "normal" AVR
arduinos, but also on the ARM-based ones, and some success has been seen
running on the ESP8266 board as well. It was tested on the Arduino Uno,
Pinoccio Scout, Teensy LC and 3.x, ESP8266, Arduino 101, Adafruit Feather M0 LoRa 900. It has been tested on the Lattice RISC-V CPU soft core running in an iCE40 UltraPlus, and also on the Murata LoRaWAN module on the MCCI Catena 4551.

This library an be quite heavy on small systems, especially if the fairly small ATmega
328p (such as in the Arduino Uno) is used. In the default configuration,
the available 32K flash space is nearly filled up (this includes some
debug output overhead, though). By disabling some features in `project_settings/lmic_project_config.h`
(like beacon tracking and ping slots, which are not needed for Class A devices),
some space can be freed up.

## Connections

To make this library work, your Arduino (or whatever Arduino-compatible
board you are using) should be connected to the transceiver. In some cases (such as the Adafruit Feather series and Murata-based boards such as the MCCI Catena 4551), the settings are fixed by the board, and you won't have to worry about many of these details. However, you'll need to find the configuration that's suitable for your board.

> To help you know if you have to worry, we'll call such boards "pre-integrated" and prefix each section with suitable guidance.

The exact
connections are a bit dependent on the transceiver board and Arduino
used, so this section tries to explain what each connection is for and
in what cases it is (not) required.

Note that the SX1272 module runs at 3.3V and likely does not like 5V on
its pins (though the datasheet is not say anything about this, and my
transceiver did not obviously break after accidentally using 5V I/O for
a few hours). To be safe, make sure to use a level shifter, or an
Arduino running at 3.3V. The Semtech evaluation board has 100 ohm resistors in
series with all data lines that might prevent damage, but I would not
count on that.

### Power

> If you're using a pre-integrated board, you can skip this section.

The SX127x transceivers need a supply voltage between 1.8V and 3.9V.
Using a 3.3V supply is typical. Some modules have a single power pin
(like the HopeRF modules, labeled 3.3V) but others expose multiple power
pins for different parts (like the Semtech evaluation board that has
`VDD_RF`, `VDD_ANA` and `VDD_FEM`), which can all be connected together.
Any *GND* pins need to be connected to the Arduino *GND* pin(s).

### SPI

> If you're using a pre-integrated board, you can skip this section, and instead refer to your board's documentation on the pins to be used.

The primary way of communicating with the transceiver is through SPI
(Serial Peripheral Interface). This uses four pins: MOSI, MISO, SCK and
SS. The former three need to be directly connected: so MOSI to MOSI,
MISO to MISO, SCK to SCK. Where these pins are located on your Arduino
varies, see for example the "Connections" section of the [Arduino SPI
documentation](SPI).

The SS (slave select) connection is a bit more flexible. On the SPI
slave side (the transceiver), this must be connect to the pin
(typically) labeled *NSS*. On the SPI master (Arduino) side, this pin
can connect to any I/O pin. Most Arduinos also have a pin labeled "SS",
but this is only relevant when the Arduino works as an SPI slave, which
is not the case here. Whatever pin you pick, you need to tell the
library what pin you used through the pin mapping (see [below](#pin-mapping)).

[SPI]: https://www.arduino.cc/en/Reference/SPI

### DIO pins

> If you're using a pre-integrated board, you can ignore this section; refer to your board's documentation for information on what DIO pins need to be used.

The DIO (digital I/O) pins on the SX127x can be configured
for various functions. The LMIC library uses them to get instant status
information from the transceiver. For example, when a LoRa transmission
starts, the DIO0 pin is configured as a TxDone output. When the
transmission is complete, the DIO0 pin is made high by the transceiver,
which can be detected by the LMIC library.

The LMIC library needs only access to DIO0, DIO1 and DIO2, the other
DIOx pins can be left disconnected. On the Arduino side, they can
connect to any I/O pin, since the current implementation does not use
interrupts or other special hardware features (though this might be
added in the feature, see also the "Timing" section).

In LoRa mode the DIO pins are used as follows:
 * DIO0: TxDone and RxDone
 * DIO1: RxTimeout

In FSK mode they are used as follows::
 * DIO0: PayloadReady and PacketSent
 * DIO2: TimeOut

Both modes need only 2 pins, but the transceiver does not allow mapping
them in such a way that all needed interrupts map to the same 2 pins.
So, if both LoRa and FSK modes are used, all three pins must be
connected.

The pins used on the Arduino side should be configured in the pin
mapping in your sketch, by setting the values of `lmic_pinmap::dio[0]`, `[1]`, and `[2]` (see [below](#pin-mapping)).

### Reset

> If you're using a pre-configured module, refer to the documentation for your board.

The transceiver has a reset pin that can be used to explicitly reset
it. The LMIC library uses this to ensure the chip is in a consistent
state at startup. In practice, this pin can be left disconnected, since
the transceiver will already be in a sane state on power-on, but
connecting it might prevent problems in some cases.

On the Arduino side, any I/O pin can be used. The pin number used must
be configured in the pin mapping `lmic_pinmap::rst` field (see [below(#pin-mapping)]).

### RXTX

> If you're using a pre-configured module, refer to the documentation for your board.

The transceiver contains two separate antenna connections: One for RX
and one for TX. A typical transceiver board contains an antenna switch
chip, that allows switching a single antenna between these RX and TX
connections.  Such a antenna switcher can typically be told what
position it should be through an input pin, often labeled *RXTX*.

The easiest way to control the antenna switch is to use the *RXTX* pin
on the SX127x transceiver. This pin is automatically set high during TX
and low during RX. For example, the HopeRF boards seem to have this
connection in place, so they do not expose any *RXTX* pins and the pin
can be marked as unused in the pin mapping.

Some boards do expose the antenna switcher pin, and sometimes also the
SX127x *RXTX* pin. For example, the SX1272 evaluation board calls the
former *FEM_CTX* and the latter *RXTX*. Again, simply connecting these
together with a jumper wire is the easiest solution.

Alternatively, or if the SX127x *RXTX* pin is not available, LMIC can be
configured to control the antenna switch. Connect the antenna switch
control pin (e.g. *FEM_CTX* on the Semtech evaluation board) to any I/O
pin on the Arduino side, and configure the pin used in the pin map (see
[below](#pin-mapping)).

The configuration entry `lmic_pinmap::rxtx` configures the pin to be used for the *RXTX* control function, in terms of the Arduino `wire.h` digital pin number. If set to `LMIC_UNUSED_PIN`, then the library assumes that software does not need to control the antenna switch.

### RXTX Polarity

If an external switch is used, you also must specify the polarity. Some modules want *RXTX* to be high for transmit, low for receive; Others want it to be low for transmit, high for receive. The Murata module, for example, requires that *RXTX* be *high* for receive, *low* for transmit.

The configuration entry `lmic_pinmap::rxtx_rx_active` should be set to the state to be written to the *RXTX* pin to make the receiver active. The opposite state is written to make the transmitter active. If `lmic_pinmap::rxtx` is `LMIC_UNUSED_PIN`, then the value of `lmic_pinmap::rxtx_rx_active` is ignored.

### Pin mapping

> For pre-configured boards, refer to the documentation on your board for the required settings. See the following:
>
> - [Adafruit Feather M0 LoRa](#adafruit-feather-m0-lora)
> - [LoRa Nexus by Ideetron](#lora-nexus-by-ideetron)
> - [MCCI Catena 4450/4460](#mcci-catena-44504460)
> - [MCCI Catena 4551](#mcci-catena-4551)

If you don't have the board documentation, you need to provide your own `lmic_pinmap` values. As described above, a variety of configurations are possible. To tell the LMIC library how your board is configured, a pin mapping struct
is used in the sketch file.

For example, this could look like this:

```c++
  lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
    // optional: set polarity of rxtx pin.
    .rxtx_rx_active = 0,
    // optional: override LMIC_SPI_FREQ if non-zero
    .spi_freq = 0,
  };
```

The names refer to the pins on the transceiver side, the numbers refer
to the Arduino pin numbers (to use the analog pins, use constants like
`A0`). For the DIO pins, the three numbers refer to DIO0, DIO1 and DIO2
respectively. Any pins that are not needed should be specified as
`LMIC_UNUSED_PIN`. The nss and dio0 pin is required, the others can
potentially left out (depending on the environments and requirements,
see the notes above for when a pin can or cannot be left out).

The name of this struct must always be `lmic_pins`, which is a special name
recognized by the library.

#### Adafruit Feather M0 LoRa

See [Feather M0 LoRa](https://www.adafruit.com/product/3178).
This board uses the following pin mapping, as shown in the various "...-feather"
sketches.

DIO0 is hard-wired by Adafruit to Arduino D3, but DIO1 is not
connected to any Arduino pin (it comes to JP1 pin 1, but is not otherwise connected). This pin table
assumes that you have manually wired JP1 pin 1 to Arduino JP3 pin 9 (Arduino D6).

DIO2 is not connected.

```c++
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
};
```

#### LoRa Nexus by Ideetron

This board uses the following pin mapping:

```c++
  const lmic_pinmap lmic_pins = {
      .nss = 10,
      .rxtx = LMIC_UNUSED_PIN,
      .rst = LMIC_UNUSED_PIN, // hardwired to AtMega RESET
      .dio = {4, 5, 7},
  };
```

#### MCCI Catena 4450/4460

See [MCCI Catena 4450](https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4450-lorawan-iot-device) and [MCCI Catena 4460](https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4460-sensor-wing-w-bme680).

These modules are based on the [Feather M0 LoRa](https://www.adafruit.com/product/3178). Since they include an extra Feather wing for the sensors, the Feather wing includes the trace connecting DIO1 to Arduino D6. No user wiring is needed on the Feather M0.

```c++
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
};
```

#### MCCI Catena 4551

See [MCCI Catena 4551](https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4551-integrated-lorawan-node).
This board uses a Murata LoRa module and has the following pin mapping:

```c++
const lmic_pinmap lmic_pins = {
    .nss = 7,
    .rxtx = 29,
    .rst = 8,
    .dio = {25, 26, 27},
    // the Murata module needs D29 high for RX, low for TX.
    .rxtx_rx_active = 1,
    // the Murata module is direct-wired, we can use 8 MHz for SPI.
    .spi_freq = 8000000
};
```

## Example Sketches

This library currently provides several examples:

 - `ttn-abp.ino` shows a basic transmission of a "Hello, world!" message
   using the LoRaWAN protocol. It contains some frequency settings and
   encryption keys intended for use with The Things Network, but these
   also correspond to the default settings of most gateways, so it
   should work with other networks and gateways as well. This example
   uses activation-by-personalization (ABP, preconfiguring a device
   address and encryption keys), and does not employ over-the-air
   activation.

   Reception of packets (in response to transmission, using the RX1 and
   RX2 receive windows is also supported).

 - `ttn-otaa.ino` also sends a "Hello, world!" message, but uses over
   the air activation (OTAA) to first join a network to establish a
   session and security keys. This was tested with The Things Network,
   but should also work (perhaps with some changes) for other networks.

 - `raw.ino` shows how to access the radio on a somewhat low level,
   and allows to send raw (non-LoRaWAN) packets between nodes directly.
   This is useful to verify basic connectivity, and when no gateway is
   available, but this example also bypasses duty cycle checks, so be
   careful when changing the settings.

 - `raw-feather.ino` is a version of `raw.ino` that is completely configured
   for the Adafruit [Feather M0 LoRa](https://www.adafruit.com/product/3178)

 - `ttn-otaa-feather-us915.ino` is a version of `ttn-otaa.ino` that has
   been configured for use with the Feather M0 LoRa, on the US915 bandplan,
   with The Things Network. Remember that you may also have to change `config.h`
   from defaults.

## Timing

Unfortunately, the SX127x transceivers do not support accurate
timekeeping themselves (there is a sequencer that is *almost* sufficient
for timing the RX1 and RX2 downlink windows, but that is only available
in FSK mode, not in LoRa mode). This means that the microcontroller is
responsible for keeping track of time. In particular, it should note
when a packet finished transmitting, so it can open up the RX1 and RX2
receive windows at a fixed time after the end of transmission.

This timing uses the Arduino `micros()` timer, which has a granularity
of 4μs and is based on the primary microcontroller clock.  For timing
events, the transceiver uses its DIOx pins as interrupt outputs. In the
current implementation, these pins are handled by an interrupt handler,
but only to set a flag - actual processing is done once every LMIC loop,
resulting in a bit inaccuracy in the timestamping. Also, running
scheduled jobs (such as opening up the receive windows) is done using a
polling approach, which might also result in further delays.

Fortunately, LoRa is a fairly slow protocol and the timing of the
receive windows is not super critical. To synchronize transmitter and
receiver, a preamble is first transmitted. Using LoRaWAN, this preamble
consists of 8 symbols, of which the receiver needs to see 4 symbols to
lock on. The current implementation tries to enable the receiver for 5
symbol times at 1.5 symbol after the start of the receive window,
meaning that a inaccuracy of plus or minus 2.5 symbol times should be
acceptable.

At the fastest LoRa setting supported by the transceiver (SF5BW500) a
single preamble symbol takes 64μs, so the receive window timing should
be accurate within 160μs (for LoRaWAN this is SF7BW250, needing accuracy
within 1280μs). This is certainly within a crystal's accuracy, but using
the internal oscillator is probably not feasible (which is 1% - 10%
accurate, depending on calibration). This accuracy should also be
feasible with the polling approach used, provided that the LMIC loop is
run often enough.

It would be good to properly review this code at some point, since it
seems that in some places some offsets and corrections are applied that
might not be appropriate for the Arduino environment. So if reception is
not working, the timing is something to have a closer look at.

The LMIC library was intended to connect the DIO pins to interrupt
lines and run code inside the interrupt handler. However, doing this
opens up an entire can of worms with regard to doing SPI transfers
inside interrupt routines (some of which is solved by the Arduino
`beginTransaction()` API, but possibly not everything). One simpler
alternative could be to use an interrupt handler to just store a
timestamp, and then do the actual handling in the main loop (this
requires modifications of the library to pass a timestamp to the LMIC
`radio_irq_handler()` function).

An even more accurate solution could be to use a dedicated timer with an
input capture unit, that can store the timestamp of a change on the DIO0
pin (the only one that is timing-critical) entirely in hardware.
Unfortunately, timer0, as used by Arduino's `millis()` and `micros()`
functions does not seem to have an input capture unit, meaning a
separate timer is needed for this.

If the main microcontroller does not have a crystal, but uses the
internal oscillator, the clock output of the transceiver (on DIO5) could
be usable to drive this timer instead of the main microcontroller clock,
to ensure the receive window timing is sufficiently accurate. Ideally,
this would use timer2, which supports asynchronous mode (e.g. running
while the microcontroller is sleeping), but that timer does not have an
input capture unit. Timer1 has one, but it seems it will stop running
once the microcontroller sleeps. Running the microcontroller in idle
mode with a slower clock might be feasible, though. Instead of using the
main crystal oscillator of the transceiver, it could be possible to use
the transceiver's internal RC oscillator (which is calibrated against
the transceiver crystal), or to calibrate the microcontroller internal
RC oscillator using the transceiver's clkout. However, that datasheet is
a bit vague on the RC oscillator's accuracy and how to use it exactly
(some registers seem to be FSK-mode only), so this needs some
experiments.

## Downlink datarate

Note that the datarate used for downlink packets in the RX2 window varies by region. Consult your network's manual for any divergences from the LoRaWAN Regional Parameters. This library assumes that the network follows the regional default.

Some networks
use different values than the specification. For example, in Europe, the specification default is DR0 (SF12, 125 kHz bandwidth). However, iot.semtech.com and The Things Network both used SF9 / 125 kHz or DR3). When using personalized activate (ABP), it is your
responsibility to set the right settings, e.g. by adding this to your
sketch (after calling `LMIC_setSession`). `ttn-abp.ino` already does
this.

```c++
LMIC.dn2Dr = DR_SF9;
```

When using OTAA, the network communicates the RX2 settings in the join accept message. This version of the LMIC library captures those settings. Therefore, you should not change the RX2 rate after joining.

## Release History

- V2.1.5 fixes issue [#56] (a documentation bug). Documentation was quickly reviewed and other issues were corrected. The OTAA examples were also updated slightly.

- V2.1.4 fixes issues [#47](https://github.com/mcci-catena/arduino-lmic/issues/47) and [#50](https://github.com/mcci-catena/arduino-lmic/issues/50) in the radio driver for the SX1276 (both related to handling of output power control bits).

- V2.1.3 has a fix for issue [#43](https://github.com/mcci-catena/arduino-lmic/issues/43): handling of `LinkAdrRequest` was incorrect for US915 and AU921; when TTN added ADR support on US and AU, the deficiency was revealed (and caused an ASSERT).

- V2.1.2 has a fix for issue [#39](https://github.com/mcci-catena/arduino-lmic/issues/39) (adding a prototype for `LMIC_DEBUG_PRINTF` if needed). Fully upward compatible, so just a patch.

- V2.1.1 has the same content as V2.1.2, but was accidentally released without updating `library.properties`.

- V2.1.0 adds support for the Murata LoRaWAN module.

- V2.0.2 adds support for the extended bandplans.

## Contributions

This library started from the IBM V1.5 open-source code.

- Thomas Telkamp and Matthijs Kooijman ported V1.5 to Arduino and did a lot of bug fixing.

- Terry Moore, LeRoy Leslie, Frank Rose, and ChaeHee Won did a lot of work on US support.

- Terry Moore added the AU921, AS923 and IN866 bandplans, and created the regionalization framework.

- [@tanupoo](https://github.com/tanupoo) of the WIDE Project debugged AS923JP and LBT support.

## Trademark Acknowledgements

LoRa is a registered trademark of the LoRa Alliance. LoRaWAN is a trademark of the LoRa Alliance.

MCCI and MCCI Catena are registered trademarks of MCCI Corporation.

All other trademarks are the properties of their respective owners.

## License

The upstream files from IBM v1.6 are based on the Berkeley license,
and the merge which synchronized this repository therefore migrated
the core files to the Berkeley license. However, modifications made
in the Arduino branch were done under the Eclipse license, so the
overall license of this repository is still Eclipse Public License
v1.0. The examples which use a more liberal
license. Some of the AES code is available under the LGPL. Refer to each
individual source file for more details, but bear in mind that until
the upstream developers look into this issue, it is safest to assume
the Eclipse license applies.
