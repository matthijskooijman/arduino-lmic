#pragma once

#ifndef _ARDUINO_LMIC_HAL_CONFIGURATION_H_
# define _ARDUINO_LMIC_HAL_CONFIGURATION_H_

#include <cstdint>

namespace ArduinoLMIC {

/* these types should match the types used by the LMIC */
typedef	int32_t	ostime_t;

// this type is used when we need to represent a threee-state signal 
enum class ThreeState_t : uint8_t {
	Off = 0,
	On = 1,
	HiZ = 2
};

// be careful of alignment below.
struct HalConfiguration_t {
	// Use this for any unused pins.
	static constexpr uint8_t UNUSED_PIN = 0xff;
	static constexpr int NUM_DIO = 3;

	virtual ostime_t setTcxoPower(uint8_t state)
		{
		// by default, if not overridden, do nothing
		// and return 0 to indicate that the caller
		// need not delay.
		return 0;
		};

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
	};

}; // end namespace ArduinoLMIC

#endif
