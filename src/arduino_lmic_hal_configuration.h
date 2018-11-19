/*

Module:  arduino_lmic_hal_configuration.h

Function:
        Arduino-LMIC C++ HAL configuration APIs

Copyright & License:
        See accompanying LICENSE file.

Author:
        Matthijs Kooijman       2015
        Terry Moore, MCCI       November 2018

*/
#pragma once

#ifndef _arduino_lmic_hal_configuration_h_
# define _arduino_lmic_hal_configuration_h_

#include <stdint.h>

namespace Arduino_LMIC {

/* these types should match the types used by the LMIC */
typedef	int32_t	ostime_t;

// this type is used when we need to represent a threee-state signal 
enum class ThreeState_t : uint8_t {
	Off = 0,
	On = 1,
	HiZ = 2
};

// forward reference
class HalConfiguration_t;

//
// for legacy reasons, we need a plain-old-data C-like
// structure that defines the "pin mapping" for the
// common pins. Many clients initialize an instance of
// this structure using named-field initialization.
//
// Be careful of alignment below.
struct HalPinmap_t {
	// Use this for any unused pins.
	static constexpr uint8_t UNUSED_PIN = 0xff;
	static constexpr int NUM_DIO = 3;
	// for backward compatibility...
	[[deprecated]]
	static constexpr uint8_t LMIC_UNUSED_PIN = UNUSED_PIN;

	/* the contents */
	uint8_t nss;		// byte 0: pin for select
	uint8_t rxtx;		// byte 1: pin for rx/tx control
	uint8_t rst;		// byte 2: pin for reset
	uint8_t dio[NUM_DIO];	// bytes 3..5: pins for DIO0, DOI1, DIO2
	// true if we must set rxtx for rx_active, false for tx_active
	uint8_t rxtx_rx_active;	// byte 6: polarity of rxtx active
	int8_t rssi_cal;	// byte 7: cal in dB -- added to RSSI
				//   measured prior to decision.
				//   Must include noise guardband!
	uint32_t spi_freq;	// bytes 8..11: SPI freq in Hz.

	// optional pointer to configuration object (byest 12..15)
	HalConfiguration_t *pConfig;
	};

class HalConfiguration_t
	{
public:
	HalConfiguration_t() {};

	// putting this into lmic_pinmap breaks old code.
	// so we have to have the weird hierarchy.
	// Old code uses lmic_pinmap and calls os_init().
	// New code uses HalConfiguration_t or other type, and
	// calls os_init_ex(). Note that code that calls 
	// os_init_ex() with lmic_pinmap must be converted.
	virtual ostime_t setTcxoPower(uint8_t state) 
		{
		// by default, if not overridden, do nothing
		// and return 0 to indicate that the caller
		// need not delay.
		return 0;
		};

	virtual void begin(void) {};
	virtual void end(void) {};
	virtual bool queryTcxoControl(void) { return false; };
	};

bool hal_init_with_pinmap(const HalPinmap_t *pPinmap);

}; // end namespace Arduino_LMIC

#endif
