# Radio Driver parameters

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

- [Radio Driver Operation](#radio-driver-operation)
	- [`os_radio(RADIO_RST)`](#os_radioradio_rst)
	- [`os_radio(RADIO_TX)`](#os_radioradio_tx)
	- [`os_radio(RADIO_RX)`](#os_radioradio_rx)
	- [`os_radio(RADIO_RXON)`](#os_radioradio_rxon)
- [Common parameters](#common-parameters)
	- [`LMIC.rps` (IN)](#lmicrps-in)
	- [`LMIC.freq` (IN)](#lmicfreq-in)
	- [`LMIC.saveIrqFlags` (OUT)](#lmicsaveirqflags-out)
	- [`LMIC.osjob` (IN/OUT)](#lmicosjob-inout)
- [Transmit parameters](#transmit-parameters)
	- [`LMIC.radio_txpow` (IN)](#lmicradio_txpow-in)
	- [`LMIC.frame[]` (IN)](#lmicframe-in)
	- [`LMIC.datalen` (IN)](#lmicdatalen-in)
	- [`LMIC.txend` (OUT)](#lmictxend-out)
- [Receive parameters](#receive-parameters)
	- [`LMIC.frame[]` (OUT)](#lmicframe-out)
	- [`LMIC.datalen` (OUT)](#lmicdatalen-out)
	- [`LMIC.rxtime` (IN/OUT)](#lmicrxtime-inout)
	- [`LMIC.lbt_ticks` (IN)](#lmiclbt_ticks-in)
	- [`LMIC.lbt_dbmax` (IN)](#lmiclbt_dbmax-in)
	- [`LMIC.rxsyms` (IN)](#lmicrxsyms-in)
	- [`LMIC.noRXIQinversion` (IN)](#lmicnorxiqinversion-in)
	- [`LMIC.snr` (OUT)](#lmicsnr-out)
	- [`LMIC.rssi` (OUT)](#lmicrssi-out)

<!-- /TOC -->
<!-- markdownlint-restore -->
<!-- Due to a bug in Markdown TOC, the table is formatted incorrectly if tab indentation is set other than 4. Due to another bug, this comment must be *after* the TOC entry. -->

## Radio Driver Operation

The LMIC radio driver operates asynchronously. Operations are started by calling the `os_radio()` function with a parameter describing the operation to be performed.

Various parameters in the LMIC structure are as input to control the operation; others are updated to return results.

### `os_radio(RADIO_RST)`

The radio is reset, and put to sleep. This operation is synchronous.

### `os_radio(RADIO_TX)`

A frame is transmitted. The parameters are given in [common parameters](#common-parameters) and [transmit parameters](#transmit-parameters).

When the operation completes, `LMIC.osjob` is scheduled.

### `os_radio(RADIO_RX)`

A single frame is received ad the specified time, and the radio is put back to sleep if no frame is found.

When the operation completes, `LMIC.osjob` is scheduled.

### `os_radio(RADIO_RXON)`

The radio is placed in continuous receive mode. If a frame is received, `LMIC.osjob` is scheduled. Continuous receive is canceled by calling [`os_radio(RADIO_RST)`](#os_radioradio_rst).

This operation is not supported in FSK mode.

### `os_radio(RADIO_TX_AT)`

This is like `os_radio(RADIO_TX)`, but the transmission is scheduled at `LMIC.txend`.

## Common parameters

### `LMIC.rps` (IN)

This is the "radio parameter setting", and it encodes several radio settings.

- Spreading factor: FSK or SF7..12
- Bandwidth: 125, 250 or 500 kHz
- Coding Rate: 4/5, 4/6, 4/7 or 4/8.
- CRC enabled/disabled
- Implicit header mode on/off. (If on, receive length must be known in advance.)

### `LMIC.freq` (IN)

This specifies the frequency, in Hertz.

### `LMIC.saveIrqFlags` (OUT)

Updated for LoRa operations only; the IRQ flags at the time of interrupt.

### `LMIC.osjob` (IN/OUT)

When asynchronous operations complete, `LMIC.osjob.func` is used as the callback function, and `LMIC.osjob` is used to schedule the work.

## Transmit parameters

### `LMIC.radio_txpow` (IN)

This specifies the transmit power in dBm.

### `LMIC.frame[]` (IN)

The array of data to be sent.

### `LMIC.datalen` (IN)

The length of the array to be sent.

### `LMIC.txend` (IN, OUT)

For `RADIO_TX_AT`, an input parameter, for the scheduled TX time.

For all transmissions, updated to the OS time at which the TX end interrupt was recognized.

## Receive parameters

### `LMIC.frame[]` (OUT)

Filled with data received.

### `LMIC.datalen` (OUT)

Set to number of bytes received in total.

### `LMIC.rxtime` (IN/OUT)

Input: When to start receiving, in OS tick time.

Output: time of RXDONE interrupt. (Note: FSK timeout doesn't currently set this cell on RX timeout.)

### `LMIC.lbt_ticks` (IN)

How long to monitor for LBT, in OS ticks.

### `LMIC.lbt_dbmax` (IN)

Maximum RSSI on channel before transmit.

### `LMIC.rxsyms` (IN)

The timeout in symbols. Only used for `os_radio(RADIO_RX)`; not used for continuous receive.

### `LMIC.noRXIQinversion` (IN)

If true, disable IQ inversion during receive.

### `LMIC.snr` (OUT)

Set to SNR * 4 after LoRa receive. (Set to 0 for FSK receive.)

### `LMIC.rssi` (OUT)

Set to RSSI + `RSSI_OFF` after LoRa receive. (Set to 0 for FSK receive; `RSSI_OFF` is 64.) You must subtract RSSI_OFF from `LMIC.rssi` to get the RSSI in dB.
