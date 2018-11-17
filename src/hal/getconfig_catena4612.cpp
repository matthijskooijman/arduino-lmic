/*

Module:  getconfig_catena4612.cpp

Function:
        Arduino-LMIC C++ HAL pinmaps for various boards

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

using namespace Arduino_LMIC;

class HalConfiguration_Catena4612_t : public HalConfiguration_t
	{
	enum DIGITAL_PINS : uint8_t
		{
		PIN_SX1276_NSS = D7,
		PIN_SX1276_NRESET = D8,
		PIN_SX1276_DIO0 = D25,
		PIN_SX1276_DIO1 = D26,
		PIN_SX1276_DIO2 = D27,
		PIN_SX1276_ANT_SWITCH_RX = D29,
		PIN_SX1276_ANT_SWITCH_TX_BOOST = D30,
		PIN_SX1276_ANT_SWITCH_TX_RFO = D31,
		PIN_VDD_BOOST_ENABLE = A0,
		PIN_TCXO_VDD = D33,
		};

	HalConfiguration_Catena4612_t() : 
		nss(D7)
		rxtx(D29)
		rst(D8)
		dio({ D25, D26, D27 })
		rxtx_active(1)
		rssi_cal(10)		/* lbt cal in dB */
		spi_freq(8000000)
		{};
	
	virtual void begin(void) override
		{
		digitalWrite(PIN_TCXO_VDD, 0);
		pinMode(PIN_TCXO_VDD, OUTPUT);
		}

	virtual void end(void) override
		{
		digitalWrite(PIN_TCXO_VDD, 0);
		pinMode(PIN_TCXO_VDD, INPUT);
		}

	virtual ostime_t setTcxoPower(uint8_t state) override
		{
		ostime_t result;
		const int oldState = digitalRead(PIN_TCXO_VDD);

		// if turning on, we need to delay.
		result = 0;
		if (state && ! oldState)
			result = ms2osticksCeil(3);

		if (state != oldState)
			digitalWrite(PIN_TCXO_VDD, state);

		return result;
		}

	
	};

static const HalConfiguration_Catena4612_t myConfig;

HalConfiguration_t