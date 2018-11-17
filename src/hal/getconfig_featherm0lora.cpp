/*

Module:  getconfig_featherm0lora.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for Adafruit Feather M0 LoRa

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

using namespace Arduino_LMIC;

class HalConfiguration_FeatherM0LoRa_t : public HalConfiguration_t
	{
	enum DIGITAL_PINS : uint8_t
		{
		PIN_SX1276_NSS = D8,
		PIN_SX1276_NRESET = D4,
		PIN_SX1276_DIO0 = D3,
		PIN_SX1276_DIO1 = D6,
		PIN_SX1276_DIO2 = UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_RX = UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_TX_BOOST = UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_TX_RFO = UNUSED_PIN,
		PIN_VDD_BOOST_ENABLE = UNUSED_PIN,
		PIN_TCXO_VDD = UNUSED_PIN,
		};

	HalConfiguration_FeatherM0LoRa_t() : 
		nss(PIN_SX1276_NSS)
		rxtx(PIN_SX1276_ANT_SWITCH_RX)
		rst(PIN_SX1276_NRESET)
		dio({ PIN_SX1276_DIO0, PIN_SX1276_DIO1, PIN_SX1276_DIO2 })
		rxtx_active(0)
		rssi_cal(8)		/* lbt cal in dB */
		spi_freq(8000000)
		{};
	
	virtual void begin(void) override
		{
		digitalWrite(PIN_NSS, 1);
		pinMode(PIN_NSS, OUTPUT);
		}

	// virtual void end(void) override

	// virtual ostime_t setTcxoPower(uint8_t state) override
	
	};

static const HalConfiguration_FeatherM0LoRa_t myConfig;

const HalConfiguration_t *GetConfig_FeatherM0Lora(void)
	{
	return &myConfig;
	}
