# Arduino-LMIC library

This repository contains the IBM LMIC (LoRaWAN-MAC-in-C) library, slightly
modified to run in the Arduino environment, allowing using the SX1272,
SX1276 transceivers and compatible modules (such as some HopeRF RFM9x
modules and the Murata LoRa modules).

This library mostly exposes the functions defined by LMIC, it makes no
attempt to wrap them in a higher level API that is more in the Arduino
style. To find out how to use the library itself, see the examples, or
see the PDF file in the doc subdirectory.

The [MCCI `arduino-lorawan`](https://github.com/mcci-catena/arduino-lorawan) library provides a higher level, more Arduino-like wrapper which may be useful.

This library requires Arduino IDE version 1.6.6 or above, since it
requires C99 mode to be enabled by default.

[![GitHub release](https://img.shields.io/github/release/mcci-catena/arduino-lmic.svg)](https://github.com/mcci-catena/arduino-lmic/releases/latest) [![GitHub commits](https://img.shields.io/github/commits-since/mcci-catena/arduino-lmic/latest.svg)](https://github.com/mcci-catena/arduino-lmic/compare/v2.3.2...master) [![Build Status](https://travis-ci.com/mcci-catena/arduino-lmic.svg?branch=master)](https://travis-ci.com/mcci-catena/arduino-lmic)

**Contents:**

<!--
  This TOC uses the VS Code markdown TOC extension AlanWalk.markdown-toc.
  We strongly recommend updating using VS Code, the markdown-toc extension and the
  bierner.markdown-preview-github-styles extension. Note that if you are using
  VS Code 1.29 and Markdown TOC 1.5.6, https://github.com/AlanWalk/markdown-toc/issues/65
  applies -- you must change your line-ending to some non-auto value in Settings>
  Text Editor>Files.  `\n` works for me.
-->
<!-- markdownlint-disable MD033 MD004 -->
<!-- markdownlint-capture -->
<!-- markdownlint-disable -->
<!-- TOC depthFrom:2 updateOnSave:true -->

- [Installing](#installing)
- [Features](#features)
- [Additional Documentation](#additional-documentation)
	- [PDF/Word Documentation](#pdfword-documentation)
	- [Adding Regions](#adding-regions)
	- [Known bugs and issues](#known-bugs-and-issues)
		- [Timing Issues](#timing-issues)
		- [Working with MCCI Murata-based boards](#working-with-mcci-murata-based-boards)
		- [Event-Handling Issues](#event-handling-issues)
- [Configuration](#configuration)
	- [Selecting the LoRaWAN Region Configuration](#selecting-the-lorawan-region-configuration)
		- [eu868, as923, in866, kr920](#eu868-as923-in866-kr920)
		- [us915, au921](#us915-au921)
	- [Selecting the target radio transceiver](#selecting-the-target-radio-transceiver)
	- [Controlling use of interrupts](#controlling-use-of-interrupts)
	- [Disabling PING](#disabling-ping)
	- [Disabling Beacons](#disabling-beacons)
	- [Enabling Network Time Support](#enabling-network-time-support)
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
		- [Disabling user events](#disabling-user-events)
		- [Disabling external reference to `onEvent()`](#disabling-external-reference-to-onevent)
		- [Enabling long messages](#enabling-long-messages)
		- [Enabling LMIC event logging calls](#enabling-lmic-event-logging-calls)
		- [Special purpose](#special-purpose)
- [Supported hardware](#supported-hardware)
- [Pre-Integrated Boards](#pre-integrated-boards)
- [Manual configuration](#manual-configuration)
	- [Power](#power)
	- [SPI](#spi)
	- [DIO pins](#dio-pins)
	- [Reset](#reset)
	- [RXTX](#rxtx)
	- [RXTX Polarity](#rxtx-polarity)
	- [Pin mapping](#pin-mapping)
		- [Advanced initialization](#advanced-initialization)
		- [HalConfiguration_t methods](#halconfiguration_t-methods)
		- [LoRa Nexus by Ideetron](#lora-nexus-by-ideetron)
- [Example Sketches](#example-sketches)
- [Timing](#timing)
	- [`LMIC_setClockError()`](#lmic_setclockerror)
- [Downlink data rate](#downlink-data-rate)
- [Encoding Utilities](#encoding-utilities)
	- [sflt16](#sflt16)
		- [JavaScript decoder](#javascript-decoder)
	- [uflt16](#uflt16)
		- [uflt16 JavaScript decoder](#uflt16-javascript-decoder)
	- [sflt12](#sflt12)
		- [sflt12f JavaScript decoder](#sflt12f-javascript-decoder)
	- [uflt12](#uflt12)
		- [uflt12f JavaScript decoder](#uflt12f-javascript-decoder)
- [Release History](#release-history)
- [Contributions](#contributions)
- [Trademark Acknowledgements](#trademark-acknowledgements)
- [License](#license)
	- [Support Open Source Hardware and Software](#support-open-source-hardware-and-software)

<!-- /TOC -->
<!-- markdownlint-restore -->
<!-- Due to a bug in Markdown TOC, the table is formatted incorrectly if tab indentation is set other than 4. Due to another bug, this comment must be *after* the TOC entry. -->

## Installing

To install this library:

- install it using the Arduino Library manager ("Sketch" -> "Include
   Library" -> "Manage Libraries..."), or
- download a zip file from GitHub using the "Download ZIP" button and
   install it using the IDE ("Sketch" -> "Include Library" -> "Add .ZIP
   Library..."
- clone this git repository into your sketchbook/libraries folder.

For more info, see [https://www.arduino.cc/en/Guide/Libraries](https://www.arduino.cc/en/Guide/Libraries).

## Features

The LMIC library provides a fairly complete LoRaWAN Class A and Class B
implementation, supporting the EU-868, US-915, AU-921, AS-923, and IN-866 bands. Only a limited
number of features was tested using this port on Arduino hardware, so be careful when using any of the untested features.

The library has only been tested with LoRaWAN 1.0.2/1.03 networks and does not have the separated key structure defined by LoRaWAN 1.1.

What certainly works:

- Sending packets uplink, taking into account duty cycling.
- Encryption and message integrity checking.
- Custom frequencies and data rate settings.
- Over-the-air activation (OTAA / joining).
- Receiving downlink packets in the RX1 and RX2 windows.
- MAC command processing.

What has not been tested:

- Class B operation.
- FSK has not been extensively tested. (Testing with the RedwoodComm RWC5020A analyzer in 2019 indicated that FSK downlink is stable but not reliable. This prevents successful completion of LoRaWAN pre-certification in regions that require support for FSK.)

If you try one of these untested features and it works, be sure to let
us know (creating a GitHub issue is probably the best way for that).

## Additional Documentation

### PDF/Word Documentation

The `doc` directory contains [LMIC-v3.0.99.pdf](doc/LMIC-v3.0.99.pdf), which documents the library APIs and use. It's based on the original IBM documentation, but has been adapted for this version of the library. However, as this library is used for more than Arduino, that document is supplemented by practical details in this document.

### Adding Regions

There is a general framework for adding support for a new region. [HOWTO-ADD-REGION.md](./HOWTO-ADD-REGION.md) has step-by-step instructions for adding a region.

### Known bugs and issues

See the list of bugs at [`mcci-catena/arduino-lmic`](https://github.com/mcci-catena/arduino-lmic/issues).

#### Timing Issues

The LoRaWAN technology for class A devices requires devices to meet hard real-time deadlines. The Arduino environment doesn't provide built-in support for this, and this port of the LMIC doesn't really ensure it, either. It is your responsibility, when constructing your application, to ensure that you call `os_runloop_once()` "often enough".

How often is often enough?

It depends on what the LMIC is doing. For Class A devices, when the LMIC is idle, `os_runloop_once()` need not be called at all. However, during a message transmit, it's critical to ensure that `os_runloop_once()` is called frequently prior to hard deadlines. The API `os_queryTimeCriticalJobs()` can be used to check whether there are any deadlines due soon. Before doing work that takes `n` milliseconds, call `os_queryTimeCriticalJobs(ms2osticks(n))`, and skip the work if the API indicates that the LMIC needs attention.

However, in the current implementation, the LMIC is tracking the completion of uplink transmits. This is done by checking for transmit-complete indications, which is done by polling. So you must also continually call `os_runloop_once()` while waiting for a transmit to be completed. This is an area for future improvement.

#### Working with MCCI Murata-based boards

The Board Support Package V2.5.0 for the MCCI Murata-based boards ([MCCI Catena 4610](https://mcci.io/catena4610), [MCCI Catena 4612](https://mcci.io/catena4612), etc.) has a defect in clock calibration that prevents the compliance script from being used without modification.  The update to V2.6.0 is expected to solve this issue.

#### Event-Handling Issues

The LMIC has a simple event notification system. When an interesting event occurs, it calls a user-provided function.

This function is sometimes called at time critical moments.

This means that your event function should avoid doing any time-critical work.

Furthermore, in versions of the LMIC prior to v3.0.99.3, the event function may be called in situations where it's not safe to call the general LMIC APIs. In those older LMIC versions, please be careful to defer all work from your event function to your `loop()` function. See the compliance example sketch for an elaborate version of how this can be done.

## Configuration

A number of features can be enabled or disabled at compile time.
This is done by adding the desired settings to the file
`project_config/lmic_project_config.h`. The `project_config`
directory is the only directory that contains files that you
should edit to match your project; we organize things this way
so that your local changes are more clearly separated from
the distribution files. The Arduino environment doesn't give
us a better way to do this, unless you change `BOARDS.txt`.

Unlike other ports of the LMIC code, in this port, you should not edit `src/lmic/config.h` to configure this package.

The following configuration variables are available.

### Selecting the LoRaWAN Region Configuration

The library supports the following regions:

`-D` variable | CFG region name | CFG region value | LoRaWAN Regional Spec 1.0.3 Reference| Frequency
------------|-----------------|:----------------:|:-------------------:|--------
`-D CFG_eu868` | `LMIC_REGION_eu868` | 1 | 2.2 | EU 863-870 MHz ISM
`-D CFG_us915` | `LMIC_REGION_us915` | 2 | 2.3 | US 902-928 MHz ISM
`-D CFG_au921` | `LMIC_REGION_au921` | 5 | 2.6 | Australia 915-928 MHz ISM
`-D CFG_as923` | `LMIC_REGION_as923` | 7 | 2.8 | Asia 923 MHz ISM
`-D CFG_as923jp` | `LMIC_REGION_as923` and `LMIC_COUNTRY_CODE_JP` | 7 | 2.8 | Asia 923 MHz ISM with Japan listen-before-talk (LBT) rules
`-D CFG_kr920` | `LMIC_REGION_kr920` | 8 | 2.9 | Korea 920-923 MHz ISM
`-D CFG_in866` | `LMIC_REGION_in866` | 9 | 2.10 | India 865-867 MHz ISM

The library requires that the compile environment or the project config file define exactly one of `CFG_...` variables. As released, `project_config/lmic_project_config.h` defines `CFG_us915`.  If you build with PlatformIO or other environments, and you do not provide a pointer to the platform config file, `src/lmic/config.h` will define `CFG_eu868`.

MCCI BSPs add menu entries to the Arduino IDE so you can select the target region interactively.

The library changes configuration pretty substantially according to the region selected, and this affects the symbols in-scope in your sketches and cpp files. Some of the differences are listed below. This list is not comprehensive, and is subject to change in future major releases.

#### eu868, as923, in866, kr920

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

If defined, configures the library to use interrupts for detecting events from the transceiver. If left undefined, the library will poll for events from the transceiver.  See [Timing](#timing) for more info.

### Disabling PING

`#define DISABLE_PING`

If defined, removes all code needed for Class B downlink during ping slots (PING).  Removes the APIs `LMIC_setPingable()` and `LMIC_stopPingable()`.
Class A devices don't support PING, so defining `DISABLE_PING` is often a good idea.

By default, PING support is included in the library.

### Disabling Beacons

`#define DISABLE_BEACONS`

If defined, removes all code needed for handling beacons. Removes the APIs `LMIC_enableTracking()` and `LMIC_disableTracking()`.

Enabling beacon handling allows tracking of network time, and is required if you want to enable downlink during ping slots. However, many networks don't support Class B devices. Class A devices don't support tracking beacons, so defining `DISABLE_BEACONS` might be a good idea.

By default, beacon support is included in the library.

### Enabling Network Time Support

`#define LMIC_ENABLE_DeviceTimeReq  number  /* boolean: 0 or non-zero */`

Disable or enable support for device network-time requests (LoRaWAN MAC request 0x0D). If zero, support is disabled. If non-zero, support is enabled.

If disabled, stub routines are provided that will return failure (so you don't need conditional compiles in client code).

### Rarely changed variables

The remaining variables are rarely used, but we list them here for completeness.

#### Changing debug output

`#define LMIC_PRINTF_TO SerialLikeObject`

This variable should be set to the name of a `Serial`-like object, used for printing messages. If not defined, `Serial`
is assumed.

#### Getting debug from the RF library

`#define LMIC_DEBUG_LEVEL number /* 0, 1, or 2 */`

This variable determines the amount of debug output to be produced by the library. The default is `0`.

If `LMIC_DEBUG_LEVEL` is zero, no output is produced. If `1`, limited output is produced. If `2`, more extensive
output is produced.  If non-zero, `printf()` is used, and the Arduino environment must be configured to support it,
otherwise the sketch will crash at runtime.

#### Selecting the AES library

The library comes with two AES implementations. The original implementation is better on
ARM processors because it's faster, but it's larger. For smaller AVR8 processors, a
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

This variable sets the default frequency for the SPI bus connection to the transceiver. The default is `1E6`, meaning 1 MHz. However, this can be overridden by the contents of the `lmic_pinmap` structure, and we recommend that you use that approach rather than editing the `project_config/lmic_project_config.h` file.

#### Changing handling of runtime assertion failures

The variables `LMIC_FAILURE_TO` and `DISABLE_LMIC_FAILURE_TO`
control the handling of runtime assertion failures. By default, assertion messages are displayed using
the `Serial` object. You can define LMIC_FAILURE_TO to be the name of some other `Print`-like object. You can
also define `DISABLE_LMIC_FAILURE_TO` to any value, in which case assert failures will silently halt execution.

#### Disabling JOIN

`#define DISABLE_JOIN`

If defined, removes code needed for OTAA activation. Removes the APIs `LMIC_startJoining()` and `LMIC_tryRejoin()`.

#### Disabling Class A MAC commands

`DISABLE_MCMD_DutyCycleReq`, `DISABLE_MCMD_RXParamSetupReq`, `DISABLE_MCMD_RXTimingSetupReq`, `DISABLE_MCMD_NewChannelReq`, and `DISABLE_MCMD_DlChannelReq` respectively disable code for various Class A MAC commands.

#### Disabling Class B MAC commands

`DISABLE_MCMD_PingSlotChannelReq` disables the PING_SET MAC commands. It's implied by `DISABLE_PING`.

`ENABLE_MCMD_BeaconTimingAns` enables the next-beacon start command. It's disabled by default, and overridden (if enabled) by `DISABLE_BEACON`. (This command is deprecated.)

#### Disabling user events

Code to handle registered callbacks for transmit, receive, and events can be suppressed by setting `LMIC_ENABLE_user_events` to zero.  This C preprocessor macro is always defined as a post-condition of `#include "config.h"`; if non-zero, user events are supported, if zero, user events are not-supported.  The default is to support user events.

#### Disabling external reference to `onEvent()`

In V3 of the LMIC, you do not need to define a function named `onEvent`. The LMIC will notice that there's no such function, and will suppress the call. However, be cautious -- in a large software package, `onEvent()` may be defined for some other purpose. The LMIC has no way of knowing that this is not the LMIC's `onEvent`, so it will call the function, and this may cause problems.

All reference to `onEvent()` can be suppressed by setting `LMIC_ENABLE_onEvent` to 0. This C preprocessor macro is always defined as a post-condition of `#include "config.h"`; if non-zero, a weak reference to `onEvent()` will be used; if zero, the user `onEvent()` function is not supported, and the client must register an event handler explicitly.  See the PDF documentation for details on `LMIC_registerEventCb()`.

#### Enabling long messages

To save RAM for simple devices, the LMIC allows message length to be limited to 64 bytes instead of the LoRaWAN standard of 255 bytes max. This saves about 2*192 bytes of RAM. Unfortunately, compliance tests require the full message size. Long messages are enabled by setting `LMIC_ENABLE_long_messages` to 1, or disabled by setting it to zero. This C preprocessor macro is always defined as a post-condition of `#include "config.h"`; if non-zero, the maximum frame size is 255 bytes, and if zero, the maximum frame size is 64 bytes.

#### Enabling LMIC event logging calls

When debugging the LMIC, debug prints change timing, and can make things not work at all. The LMIC has embedded optional calls to capture debug information that can be printed out later, when the LMIC is not active. Logging is enabled by setting `LMIC_ENABLE_event_logging` to 1. The default is not to log. This C preprocessor macro is always defined as a post-condition of `#include "config.h"`.

The compliance test script includes a suitable logging implementation; the other example scripts do not.

#### Special purpose

`#define DISABLE_INVERT_IQ_ON_RX` disables the inverted Q-I polarity on RX. **Use of this variable is deprecated, see issue [#250](https://github.com/mcci-catena/arduino-lmic/issues/250).** Rather than defining this, set the value of `LMIC.noRXIQinversion`. If set non-zero, receive will be non-inverted. End-devices will be able to receive messages from each other, but will not be able to hear the gateway (other than Class B beacons)aa. If set zero, (the default), end devices will only be able to hear gateways, not each other.

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
should be architecture-independent. Users have tested this on AVR, ARM, Xtensa-based, and RISC-V based system.

This library an be quite heavy on small systems, especially if the fairly small ATmega
328p (such as in the Arduino Uno) is used. In the default configuration,
the available 32K flash space is nearly filled up (this includes some
debug output overhead, though). By disabling some features in `project_config/lmic_project_config.h`
(like beacon tracking and ping slots, which are not needed for Class A devices),
some space can be freed up.

## Pre-Integrated Boards

There are two ways of using this library, either with pre-integrated boards or with manually configured boards.

The following boards are pre-integrated.

- Adafruit [Feather 32u4 LoRa 900 MHz][1] (SX1276)
- Adafruit [Feather M0 LoRa 900 MHz][2] (SX1276)
- MCCI Catena 4410, 4420, [4450][3], [4460][4] and [4470][5] boards (based on Adafruit Feather boards plus wings) (SX1276)
- MCCI Catena 4551, [4610][6], 4611, [4612][7], 4617, 4618, 4630, and [4801][8] boards (based on the Murata CMWX1ZZABZ-078 module) (SX1276)

[1]: https://www.adafruit.com/products/3078
[2]: https://www.adafruit.com/products/3178
[3]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4450-lorawan-iot-device
[4]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4460-sensor-wing-w-bme680
[5]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/mcci-catena-4470-modbus-node-for-lorawan-technology
[6]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/mcci-catena-4610-integrated-node-for-lorawan-technology
[7]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4612-integrated-lorawan-node
[8]: https://store.mcci.com/collections/lorawan-iot-and-the-things-network/products/catena-4801

> To help you know if you have to worry, we'll call such boards "pre-integrated" and prefix each section with suitable guidance.

## Manual configuration

If your desired transceiver board is not pre-integrated, you need to provide the library with the required information.

You may need to wire up your transceiver. The exact
connections are a bit dependent on the transceiver board and Arduino
used, so this section tries to explain what each connection is for and
in what cases it is (not) required.

Note that the SX127x module runs at 3.3V and likely does not like 5V on
its pins (though the datasheet is not say anything about this, and my
transceiver did not obviously break after accidentally using 5V I/O for
a few hours). To be safe, make sure to use a level shifter, or an
Arduino running at 3.3V. The Semtech evaluation board has 100 ohm resistors in
series with all data lines that might prevent damage, but I would not
count on that.

### Power

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

The SX127x transceivers need a supply voltage between 1.8V and 3.9V.
Using a 3.3V supply is typical. Some modules have a single power pin
(like the HopeRF modules, labeled 3.3V) but others expose multiple power
pins for different parts (like the Semtech evaluation board that has
`VDD_RF`, `VDD_ANA` and `VDD_FEM`), which can all be connected together.
Any *GND* pins need to be connected to the Arduino *GND* pin(s).

### SPI

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

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

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

The DIO (digital I/O) pins on the SX127x can be configured
for various functions. The LMIC library uses them to get instant status
information from the transceiver. For example, when a LoRa transmission
starts, the DIO0 pin is configured as a TxDone output. When the
transmission is complete, the DIO0 pin is made high by the transceiver,
which can be detected by the LMIC library.

The LMIC library needs only access to DIO0, DIO1 and DIO2, the other
DIOx pins can be left disconnected. On the Arduino side, they can
connect to any I/O pin. If interrupts are used, the accuracy of timing
will be improved, particularly the rest of your `loop()` function has
lengthy calculations; but in that case, the enabled DIO pins must all
support rising-edge interrupts. See the [Timing](#timing) section below.

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

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

The transceiver has a reset pin that can be used to explicitly reset
it. The LMIC library uses this to ensure the chip is in a consistent
state at startup. In practice, this pin can be left disconnected, since
the transceiver will already be in a sane state on power-on, but
connecting it might prevent problems in some cases.

On the Arduino side, any I/O pin can be used. The pin number used must
be configured in the pin mapping `lmic_pinmap::rst` field (see [below](#pin-mapping)).

### RXTX

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

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

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

If an external switch is used, you also must specify the polarity. Some modules want *RXTX* to be high for transmit, low for receive; Others want it to be low for transmit, high for receive. The Murata module, for example, requires that *RXTX* be *high* for receive, *low* for transmit.

The configuration entry `lmic_pinmap::rxtx_rx_active` should be set to the state to be written to the *RXTX* pin to make the receiver active. The opposite state is written to make the transmitter active. If `lmic_pinmap::rxtx` is `LMIC_UNUSED_PIN`, then the value of `lmic_pinmap::rxtx_rx_active` is ignored.

### Pin mapping

> If you're using a [pre-integrated board](#pre-integrated-boards), you can skip this section.

Refer to the documentation on your board for the required settings.

Remember, for pre-integrated boards, you don't worry about this.

We have details for the following manually-configured boards here:

- [LoRa Nexus by Ideetron](#lora-nexus-by-ideetron)

If your board is not configured, you need at least to provide your own `lmic_pinmap`. As described above, a variety of configurations are possible. To tell the LMIC library how your board is configured, you must declare a variable containing a pin mapping struct in your sketch file.  If you call `os_init()` to initialize the LMIC, you must name this structure `lmic_pins`. If you call `os_init_ex()`, you may name the structure what you like, but you pass a pointer as the parameter to `os_init_ex()`.

Here's an example of a simple initialization:

```c++
  lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
    // optional: set polarity of rxtx pin.
    .rxtx_rx_active = 0,
    // optional: set RSSI cal for listen-before-talk
    // this value is in dB, and is added to RSSI
    // measured prior to decision.
    // Must include noise guardband! Ignored in US,
    // EU, IN, other markets where LBT is not required.
    .rssi_cal = 0,
    // optional: override LMIC_SPI_FREQ if non-zero
    .spi_freq = 0,
  };
```

The names refer to the pins on the transceiver side, the numbers refer
to the Arduino pin numbers (to use the analog pins, use constants like
`A0`). For the DIO pins, the three numbers refer to DIO0, DIO1 and DIO2
respectively. Any pins that are not needed should be specified as
`LMIC_UNUSED_PIN`. The NSS and dio0 pins are required. The others can
potentially left out (depending on the environments and requirements,
see the notes above for when a pin can or cannot be left out).

#### Advanced initialization

In some boards require much more advanced management. The LMIC has a very flexible framework to support this, but it requires you to do some C++ work.

1. You must define a new class derived from `Arduino_LMIC::HalConfiguration_t`. (We'll call this `cMyHalConfiguration_t`).

2. This class *may* define overrides for several methods (discussed below).

3. You must create an instance of your class, e.g.

    ```c++
    cMyHalConfiguration_t myHalConfigInstance;
    ```

4. You add another entry in your `lmic_pinmap`, `pConfig = &myHalConfigInstance`, to link your pin-map to your object.

The full example looks like this:

```c++
class cMyHlaConfiguration_t : public Arduino_LMIC::HalConfiguration_t
  {
public:
  // ...
  // put your method function override declarations here.

  // this example uses RFO at 10 dBm or less, PA_BOOST up to 17 dBm,
  // or the high-power mode above 17 dBm. In other words, it lets the
  // LMIC-determined policy determine what's to be done.

  virutal TxPowerPolicy_t getTxPowerPolicy(
    TxPowerPolicy_t policy,
    int8_t requestedPower,
    uint32_t frequency
    ) override
    {
    return policy;
    }
  }
```

#### HalConfiguration_t methods

- `ostime_t setModuleActive(bool state)` is called by the LMIC to make the module active or to deactivate it (the value of `state` is true to activate).  The implementation must turn power to the module on and otherwise prepare for it to go to work, and must return the number of OS ticks to wait before starting to use the radio.

- `void begin(void)` is called during initialization, and is your code's chance to do any early setup.

- `void end(void)` is (to be) called during late shutdown.  (Late shutdown is not implemented yet; but we wanted to add the API for consistency.)

- `bool queryUsingTcxo(void)` shall return `true` if the module uses a TCXO; `false` otherwise.

- `TxPowerPolicy_t getTxPowerPolicy(TxPowerPolicy_t policy, int8_t requestedPower, uint32_t frequency)` allows you to override the LMIC's selection of transmit power. If not provided, the default method forces the LMIC to use PA_BOOST mode. (We chose to do this because we found empirically that the Hope RF module doesn't support RFO, and because legacy LMIC code never used anything except PA_BOOST mode.)

Caution: the LMIC has no way of knowing whether the mode you return makes sense. Use of 20 dBm mode without limiting duty cycle can over-stress your module. The LMIC currently does not have any code to duty-cycle US transmissions at 20 dBm. If properly limiting transmissions to 400 milliseconds, a 1% duty-cycle means at most one message every 40 seconds. This shouldn't be a problem in practice, but buggy upper level software still might do things more rapidly.

<!-- there are links to the following section, so be careful when renaming -->
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

## Example Sketches

This library provides several examples.

- [`ttn-otaa.ino`](examples/ttn-otaa/ttn-otaa.ino) shows a basic transmission of a "Hello, world!" message
   using the LoRaWAN protocol. It contains some frequency settings and
   encryption keys intended for use with The Things Network, but these
   also correspond to the default settings of most gateways, so it
   should work with other networks and gateways as well.
   The example uses over-the-air activation (OTAA) to first join the network to establish a
   session and security keys. This was tested with The Things Network,
   but should also work (perhaps with some changes) for other networks.
   OTAA is the preferred way to work with production LoRaWAN networks.

- [`ttn-otaa-feather-us915.ino`](examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino) is a version of `ttn-otaa.ino` that has
   been configured for use with the Feather M0 LoRa, in the US915 region,
   with The Things Network. Remember that you may also have to change `config.h`
   from defaults. This sketch also works with the MCCI Catena family of products
   as well as with the Feather 32u4 LoRa.

- [`ttn-otaa-feather-us915-dht22.ino`](examples/ttn-otaa-feather-us915-dht22/ttn-otaa-feather-us915-dht22.ino)
   is a further refinement of `ttn-otaa-feather-us915.ino`. It measures and
   transmits temperature and relative humidity using a DHT22 sensor. It's only
   been tested with Feather M0-family products.

- [`raw.ino`](examples/raw/raw.ino) shows how to access the radio on a somewhat low level,
   and allows to send raw (non-LoRaWAN) packets between nodes directly.
   This is useful to verify basic connectivity, and when no gateway is
   available, but this example also bypasses duty cycle checks, so be
   careful when changing the settings.

- [`raw-feather.ino`](examples/raw-feather/raw-feather.ino) is a version of `raw.ino` that is completely configured
   for the Adafruit [Feather M0 LoRa](https://www.adafruit.com/product/3178), and for a variety
   of other MCCI products.

- [`ttn-abp.ino`](examples/ttn-abp/ttn-abp.ino) shows a basic transmission of a "Hello, world!" message
   using the LoRaWAN protocol. This example
   uses activation-by-personalization (ABP, preconfiguring a device
   address and encryption keys), and does not employ over-the-air
   activation.

   ABP should not be used if you have access to a production gateway and network; it's
   not compliant with LoRaWAN standards, it's not FCC compliant, and it's uses spectrum
   in a way that's unfair to other users. However, it's often the most economical way to
   get your feet wet with this technology. It's possible to do ABP compliantly with the LMIC
   framework, but you need to have FRAM storage and a framework that saves uplink and
   downlink counts across reboots and resets. See, for example,
   [Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform).

- [`ttn-abp-feather-us915-dht22.ino`](examples/ttn-abp-feather-us915-dht22/ttn-abp-feather-us915-dht22.ino)
   refines `ttn-abp.ino` by configuring for use with the Feather M0 LoRa in the US915 region,
   with a single-channel gateway on The Things Network; it measures and transmits temperature and relative
   humidity using a DHT22 sensor. It's only been tested with Feather M0-family products.

   ABP should not be used if you have access to a production gateway and network; it's
   not compliant with LoRaWAN standards, it's not FCC compliant, and it's uses spectrum
   in a way that's unfair to other users. However, it's often the most economical way to
   get your feet wet with this technology.  It's possible to do ABP compliantly with the LMIC
   framework, but you need to have FRAM storage and a framework that saves uplink and
   downlink counts across reboots and resets. See, for example,
   [Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform).

- [`header_test.ino`](examples/header_test/header_test.ino) just tests the header files; it's used for regression testing.

## Timing

The library is
responsible for keeping track of time of certain network events, and scheduling
other events relative to those events. In particular, the library must note
when a packet finishes transmitting, so it can open up the RX1 and RX2
receive windows at a fixed time after the end of transmission. The library does this
by watching for rising edges on the DIO0 output of the SX127x, and noting the time.

The library observes and processes rising edges on the pins as part of `os_runloop()` processing.
This can be configured in one of two ways (see
[Controlling use of interrupts](#controlling-use-of-interrupts)).

By default, the routine `hal_io_check()`
polls the enabled pins to determine whether an event has occurred. This approach
allows use of any CPU pin to sense the DIOs, and makes no assumptions about
interrupts. However, it means that the end-of-transmit event is not observed
(and time-stamped) until `os_runloop()` is called.

Optionally, you can configure the LMIC library to use interrupts. The
interrupt handlers capture the time of
the event. Actual processing is done the next time that `os_runloop()`
is called, using the captured time. However, this requires that the
DIO pins be wired to Arduino pins that support rising-edge interrupts.

Fortunately, LoRa is a fairly slow protocol and the timing of the
receive windows is not super critical. To synchronize transmitter and
receiver, a preamble is first transmitted. Using LoRaWAN, this preamble
consists of 8 symbols, of which the receiver needs to see 4 symbols to
lock on. The current implementation tries to enable the receiver for 5
symbol times at 1.5 symbol after the start of the receive window,
meaning that a inaccuracy of plus or minus 2.5 symbol times should be
acceptable.

The HAL bases all timing on the Arduino `micros()` timer, which has a platform-specific
granularity, and is based on the primary microcontroller clock.

At the fastest LoRa setting supported by the SX127x (SF5BW500) a
single preamble symbol takes 64 microseconds, so the receive window timing should
be accurate within 160 microseconds (for LoRaWAN this is SF7BW250, needing accuracy
within 1280μs). This accuracy should also be
feasible with the polling approach used, provided that the LMIC loop is
run often enough.

If using an internal oscillator (which is 1% - 10%
accurate, depending on calibration), or if your other `loop()` processing
is time consuming, you may have to use [`LMIC_setClockError()`](#lmic_setclockerror)
to cause the library to leave the radio on longer.

An even more accurate solution could be to use a dedicated timer with an
input capture unit, that can store the timestamp of a change on the DIO0
pin (the only one that is timing-critical) entirely in hardware.
Experience shows that this is not normally required, so we leave this as
a customization to be performed on a platform-by-platform basis. We provide
a special API, `radio_irq_handler_v2(u1_t dio, ostime_t tEvent)`. This
API allows you to supply a hardware-captured time for extra accuracy.

The practical consequence of inaccurate timing is reduced battery life;
the LMIC must turn on the receiver earlier in order to be sure to capture downlink packets.

### `LMIC_setClockError()`

You may call this routine during initialization to inform the LMIC code about the timing accuracy of your system.

```c++
enum { MAX_CLOCK_ERROR = 65535 };

void LMIC_setClockError(
    u2_t error
);
```

This function sets the anticipated relative clock error. `MAX_CLOCK_ERROR`
represents +/- 100%, and 0 represents no additional clock compensation.
To allow for an error of 20%, you would call

```c++
LMIC_setClockError(MAX_CLOCK_ERROR * 20 / 100);
```

Setting a high clock error causes the RX windows to be opened earlier than it otherwise would be. This causes more power to be consumed. For Class A devices, this extra power is not substantial, but for Class B devices, this can be significant.

This clock error is not reset by `LMIC_reset()`.

## Downlink data rate

Note that the data rate used for downlink packets in the RX2 window varies by region. Consult your network's manual for any divergences from the LoRaWAN Regional Parameters. This library assumes that the network follows the regional default.

Some networks use different values than the specification. For example, in Europe, the specification default is DR0 (SF12, 125 kHz bandwidth). However, iot.semtech.com and The Things Network both used SF9 / 125 kHz or DR3). If using over-the-air activation (OTAA), the network will download RX2 parameters as part of the JoinAccept message; the LMIC will honor the downloaded parameters.

However, when using personalized activate (ABP), it is your
responsibility to set the right settings, e.g. by adding this to your
sketch (after calling `LMIC_setSession`). `ttn-abp.ino` already does
this.

```c++
LMIC.dn2Dr = DR_SF9;
```

## Encoding Utilities

It is generally important to make LoRaWAN messages as small as practical. Extra bytes mean extra transmit time, which wastes battery power and interferes with other nodes on the network.

To simplify coding, the Arduino header file <lmic.h> defines some data encoding utility functions to encode floating-point data into `uint16_t` values using `sflt16` or `uflt16` bit layout. For even more efficiency, there are versions that use only the bottom 12 bits of the `uint16_t`, allowing for other bits to be carried in the top 4 bits, or for two values to be crammed into three bytes.

- `uint16_t LMIC_f2sflt16(float)` converts a floating point number to a [`sflt16`](#sflt16)-encoded `uint16_t`.
- `uint16_t LMIC_f2uflt16(float)` converts a floating-point number to a [`uflt16`](#uflt16)-encoded `uint16_t`.
- `uint16_t LMIC_f2sflt12(float)` converts a floating-point number to a [`sflt12`](#sflt12)-encoded `uint16_t`, leaving the top four bits of the result set to zero.
- `uint16_t LMIC_f2uflt12(float)` converts a floating-point number to a [`uflt12`](#sflt12)-encoded `uint16_t`, leaving the top four bits of the result set to zero.

JavaScript code for decoding the data can be found in the following sections.

### sflt16

A `sflt16` datum represents an unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
15 | Sign bit
14..11 | binary exponent `b`
10..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/2048 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated. However, it is similar to IEEE format in that it uses sign-magnitude rather than twos-complement for negative values.

For example, if the data value is 0x8D, 0x55, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x8D55.
2. Bit 15 is 1, so this is a negative value.
3. `b`  is 1, and `b`-15 is -14.  2^-14 is 1/16384
4. `f` is 0x555. 0x555/2048 = 1365/2048 is 0.667
5. `f * 2^(b-15)` is therefore 0.667/16384 or 0.00004068
6. Since the number is negative, the value is -0.00004068

Floating point mavens will immediately recognize:

* This format uses sign/magnitude representation for negative numbers.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 2048..4095, and then transmit only `f - 2048`, saving a bit. However, this complicates the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

#### JavaScript decoder

```javascript
function sflt162f(rawSflt16)
    {
    // rawSflt16 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFFF
    // bit 15 is the sign bit
    // bits 14..11 are the exponent
    // bits 10..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the open interval (-1.0, 1.0);
    //

    // throw away high bits for repeatability.
    rawSflt16 &= 0xFFFF;

    // special case minus zero:
    if (rawSflt16 == 0x8000)
        return -0.0;

    // extract the sign.
    var sSign = ((rawSflt16 & 0x8000) != 0) ? -1 : 1;

    // extract the exponent
    var exp1 = (rawSflt16 >> 11) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawSflt16 & 0x7FF) / 2048.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

### uflt16

A `uflt16` datum represents an unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
15..12 | binary exponent `b`
11..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/4096 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0xEB, 0xF7, and the transmitted byte order is big endian, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0xEBF7.
2. `b`  is therefore 0xE, and `b`-15 is -1.  2^-1 is 1/2
3. `f` is 0xBF7. 0xBF7/4096 is 3063/4096 == 0.74780...
4. `f * 2^(b-15)` is therefore 0.74780/2 or 0.37390

Floating point mavens will immediately recognize:

* There is no sign bit; all numbers are positive.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 4096..8191, and then transmit only `f - 4096`, saving a bit. However, this complicated the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

#### uflt16 JavaScript decoder

```javascript
function uflt162f(rawUflt16)
    {
    // rawUflt16 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFFF
    // bits 15..12 are the exponent
    // bits 11..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the half-open interval [0, 1.0);
    //

    // throw away high bits for repeatability.
    rawUflt16 &= 0xFFFF;

    // extract the exponent
    var exp1 = (rawUflt16 >> 12) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawUflt16 & 0xFFF) / 4096.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

### sflt12

A `sflt12` datum represents an signed floating point number in the range [0, 1.0), transmitted as a 12-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
11 | sign bit
11..8 | binary exponent `b`
7..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/128 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0x8, 0xD5, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x8D5.
2. The number is negative.
3. `b` is 0x1, and `b`-15 is -14.  2^-14 is 1/16384
4. `f` is 0x55. 0x55/128 is 85/128, or 0.66
5. `f * 2^(b-15)` is therefore 0.66/16384 or 0.000041 (to two significant digits)
6. The decoded number is therefore -0.000041.

Floating point mavens will immediately recognize:

* This format uses sign/magnitude representation for negative numbers.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 128 .. 256, and then transmit only `f - 128`, saving a bit. However, this complicates the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.
* It can be strongly argued that dropping the sign bit would be worth the effort, as this would get us 14% more resolution for a minor amount of work.

#### sflt12f JavaScript decoder

```javascript
function sflt12f(rawSflt12)
    {
    // rawSflt12 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFF (12 bits). For safety, we mask
    // on entry and discard the high-order bits.
    // bit 11 is the sign bit
    // bits 10..7 are the exponent
    // bits 6..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the open interval (-1.0, 1.0);
    //

    // throw away high bits for repeatability.
    rawSflt12 &= 0xFFF;

    // special case minus zero:
    if (rawSflt12 == 0x800)
        return -0.0;

    // extract the sign.
    var sSign = ((rawSflt12 & 0x800) != 0) ? -1 : 1;

    // extract the exponent
    var exp1 = (rawSflt12 >> 7) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawSflt12 & 0x7F) / 128.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

### uflt12

A `uflt12` datum represents an unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
11..8 | binary exponent `b`
7..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/256 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0x1, 0xAB, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x1AB.
2. `b`  is therefore 0x1, and `b`-15 is -14.  2^-14 is 1/16384
3. `f` is 0xAB. 0xAB/256 is 0.67
4. `f * 2^(b-15)` is therefore 0.67/16384 or 0.0000408 (to three significant digits)

Floating point mavens will immediately recognize:

* There is no sign bit; all numbers are positive.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 256 .. 512, and then transmit only `f - 256`, saving a bit. However, this complicates the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

#### uflt12f JavaScript decoder

```javascript
function uflt12f(rawUflt12)
    {
    // rawUflt12 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFF (12 bits). For safety, we mask
    // on entry and discard the high-order bits.
    // bits 11..8 are the exponent
    // bits 7..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the half-open interval [0, 1.0);
    //

    // throw away high bits for repeatability.
    rawUflt12 &= 0xFFF;

    // extract the exponent
    var exp1 = (rawUflt12 >> 8) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawUflt12 & 0xFF) / 256.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

## Release History

- v3.0.99 (still in pre-release) adds the following changes. (This is not an exhaustive list.) Note that the behavior of the LMIC changes in important ways, as it now enforces the LoRaWAN mandated maximum frame size for a given data rate. For Class A devices, this may cause your device to go silent after join, if you're not able to handle the frame size dictated by the parameters downloaded to the device by the network during join. The library will attempt to find a data rate that will work, but there is no guarantee that the network has provided such a data rate.

  - [#452](https://github.com/mcci-catena/arduino-lmic/pull/452) fixes a bug [#450](https://github.com/mcci-catena/arduino-lmic/issues/450) in `LMIC_clrTxData()` that would cause join hiccups if called while (1) a join was in progress and (2) a regular data packet was waiting to be uplinked after the join completes. Also fixes AS923- and AU915-specific bugs [#446](https://github.com/mcci-catena/arduino-lmic/issues/446), [#447](https://github.com/mcci-catena/arduino-lmic/issues/447), [#448](https://github.com/mcci-catena/arduino-lmic/issues/448). Version is `v3.0.99.5`.
  - [#443](https://github.com/mcci-catena/arduino-lmic/pull/443) addresses a number of problems found in cooperation with [RedwoodComm](https://redwoodcomm.com). They suggested a timing improvement to speed testing; this lead to the discovery of a number of problems. Some were in the compliance framework, but one corrects timing for very high spreading factors, several ([#442](https://github.com/mcci-catena/arduino-lmic/issues/442), [#436](https://github.com/mcci-catena/arduino-lmic/issues/438), [#435](https://github.com/mcci-catena/arduino-lmic/issues/435), [#434](https://github.com/mcci-catena/arduino-lmic/issues/434) fix glaring problems in FSK support; [#249](https://github.com/mcci-catena/arduino-lmic/issues/249) greatly enhances stability by making API calls much less likely to crash the LMIC if it's active. Version is v3.0.99.3.
  - [#388](https://github.com/mcci-catena/arduino-lmic/issues/388), [#389](https://github.com/mcci-catena/arduino-lmic/issues/390), [#390](https://github.com/mcci-catena/arduino-lmic/issues/390) change the LMIC to honor the maximum frame size for a given DR in the current region. This proves to be a breaking change for many applications, especially in the US, because DR0 in the US supports only an 11-byte payload, and many apps were ignoring this. Additional error codes were defined so that apps can detect and recover from this situation, but they must detect; otherwise they run the risk of being blocked from the network by the LMIC.  Because of this change, the next version of the LMIC will be V3.1 or higher, and the LMIC version for development is bumped to 3.0.99.0.
  - [#401](https://github.com/mcci-catena/arduino-lmic/issues/401) adds 865 MHz through 868 MHz to the "1%" band for EU.
  - [#395](https://github.com/mcci-catena/arduino-lmic/pull/395) corrects pin-mode initialization if using `hal_interrupt_init()`.
  - [#385](https://github.com/mcci-catena/arduino-lmic/issues/385) corrects an error handling data rate selection for `TxParamSetupReq`, found in US-915 certification testing. (v2.3.2.71)
  - [#378](https://github.com/mcci-catena/arduino-lmic/pull/378) completely reworks MAC downlink handling. Resulting code passes the LoRaWAN V1.5 EU certification test. (v2.3.2.70)
  - [#360](https://github.com/mcci-catena/arduino-lmic/issues/360) adds support for the KR-920 regional plan.

- v2.3.2 is a patch release. It incorporates two pull requests.

  - [#204](https://github.com/mcci-catena/arduino-lmic/pull/204) eliminates a warning if using a custom pin-map.
  - [#206](https://github.com/mcci-catena/arduino-lmic/pull/206) updates CI testing to Arduino IDE v1.8.8.

- v2.3.1 is a patch release. It adds `<arduino_lmic_user_configuration.h>`, which loads the pre-processor LMIC configuration variables into scope (issue [#199](https://github.com/mcci-catena/arduino-lmic/issues/199)).

- v2.3.0 introduces two important changes.

    1. The pin-map is extended with an additional field `pConfig`, pointing to a C++ class instance. This instance, if provided, has extra methods for dealing with TCXO control and other fine details of operating the radio. It also gives a natural way for us to extend the behavior of the HAL.

    2. Pinmaps can be pre-configured into the library, so that users don't have to do this in every sketch.

  Accompanying this was a fairly large refactoring of inner header files. We now have top-level header file `<arduino_lmic_hal_configuration.h>`, which provides much the same info as the original `<hal/hal.h>`, without bringing most of the LMIC internal definitions into scope. We also changed the SPI API based on a suggestion from `@manuelbl`, making the HAL more friendly to structured BSPs (and also making the SPI API potentially faster).

- Interim bug fixes: added a new API (`radio_irq_handler_v2()`), which allows the caller to provide the timestamp of the interrupt. This allows for more accurate timing, because the knowledge of interrupt overhead can be moved to a platform-specific layer ([#148](https://github.com/mcci-catena/arduino-lmic/issues/148)). Fixed compile issues on ESP32 ([#140](https://github.com/mcci-catena/arduino-lmic/issues/140) and [#153](https://github.com/mcci-catena/arduino-lmic/issues/150)). We added ESP32 and 32u4 as targets in CI testing. We switched CI testing to Arduino IDE 1.8.7.
   Fixed issue [#161](https://github.com/mcci-catena/arduino-lmic/issues/161) selecting the Japan version of as923 using `CFG_as923jp` (selecting via `CFG_as923` and `LMIC_COUNTRY_CODE=LMIC_COUNTRY_CODE_JP` worked).
   Fixed [#38](https://github.com/mcci-catena/arduino-lmic/issues/38) -- now any call to hal_init() will put the NSS line in the idle (high/inactive) state. As a side effect, RXTX is initialized, and RESET code changed to set value before transitioning state. Likely no net effect, but certainly more correct.

- V2.2.2 adds `ttn-abp-feather-us915-dht22.ino` example, and fixes some documentation typos. It also fixes encoding of the `Margin` field of the `DevStatusAns` MAC message ([#130](https://github.com/mcci-catena/arduino-lmic/issues/130)).  This makes Arduino LMIC work with networks implemented with [LoraServer](https://www.loraserver.io/).

- V2.2.1 corrects the value of `ARDUINO_LMIC_VERSION` ([#123](https://github.com/mcci-catena/arduino-lmic/issues/123)), allows ttn-otaa-feather-us915 example to compile for the Feather 32u4 LoRa ([#116](https://github.com/mcci-catena/arduino-lmic/issues/116)), and addresses documentation issues ([#122](https://github.com/mcci-catena/arduino-lmic/issues/122), [#120](https://github.com/mcci-catena/arduino-lmic/issues/120)).

- V2.2.0 adds encoding functions and `tn-otaa-feather-us915-dht22.ino` example. Plus a large number of issues: [#59](https://github.com/mcci-catena/arduino-lmic/issues/59), [#60](https://github.com/mcci-catena/arduino-lmic/issues/60), [#63](https://github.com/mcci-catena/arduino-lmic/issues/63), [#64](https://github.com/mcci-catena/arduino-lmic/issues/47) (listen-before-talk for Japan), [#65](https://github.com/mcci-catena/arduino-lmic/issues/65), [#68](https://github.com/mcci-catena/arduino-lmic/issues/68), [#75](https://github.com/mcci-catena/arduino-lmic/issues/75), [#78](https://github.com/mcci-catena/arduino-lmic/issues/78), [#80](https://github.com/mcci-catena/arduino-lmic/issues/80), [#91](https://github.com/mcci-catena/arduino-lmic/issues/91), [#98](https://github.com/mcci-catena/arduino-lmic/issues/98), [#101](https://github.com/mcci-catena/arduino-lmic/issues/101). Added full Travis CI testing, switched to travis-ci.com as the CI service. Prepared to publish library in the official Arduino library list.

- V2.1.5 fixes issue [#56](https://github.com/mcci-catena/arduino-lmic/issues/56) (a documentation bug). Documentation was quickly reviewed and other issues were corrected. The OTAA examples were also updated slightly.

- V2.1.4 fixes issues [#47](https://github.com/mcci-catena/arduino-lmic/issues/47) and [#50](https://github.com/mcci-catena/arduino-lmic/issues/50) in the radio driver for the SX1276 (both related to handling of output power control bits).

- V2.1.3 has a fix for issue [#43](https://github.com/mcci-catena/arduino-lmic/issues/43): handling of `LinkAdrRequest` was incorrect for US915 and AU921; when TTN added ADR support on US and AU, the deficiency was revealed (and caused an ASSERT).

- V2.1.2 has a fix for issue [#39](https://github.com/mcci-catena/arduino-lmic/issues/39) (adding a prototype for `LMIC_DEBUG_PRINTF` if needed). Fully upward compatible, so just a patch.

- V2.1.1 has the same content as V2.1.2, but was accidentally released without updating `library.properties`.

- V2.1.0 adds support for the Murata LoRaWAN module.

- V2.0.2 adds support for additional regions.

## Contributions

This library started from the IBM V1.5 open-source code.

- Thomas Telkamp and Matthijs Kooijman ported V1.5 to Arduino and did a lot of bug fixing.

- Terry Moore, LeRoy Leslie, Frank Rose, and ChaeHee Won did a lot of work on US support.

- Terry Moore added the AU921, AS923, KR920 and IN866 regions, and created the regionalization framework, and did corrections for LoRaWAN 1.0.3 compliance testing.

- [`@tanupoo`](https://github.com/tanupoo) of the WIDE Project debugged AS923JP and LBT support.

## Trademark Acknowledgements

LoRa is a registered trademark of Semtech Corporation. LoRaWAN is a registered trademark of the LoRa Alliance.

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

### Support Open Source Hardware and Software

MCCI invests time and resources providing this open source code, please support MCCI and open-source hardware by purchasing products from MCCI, Adafruit and other open-source hardware/software vendors!

For information about MCCI's products, please visit [store.mcci.com](https://store.mcci.com/).
