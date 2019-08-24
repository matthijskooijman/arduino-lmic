/*

Module:  getconfig_feather32u4lora.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for Adafruit Feather 32U4 LoRa

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

namespace Arduino_LMIC {

class HalConfiguration_Feather32U4LoRa_t : public HalConfiguration_t
	{
public:
	enum DIGITAL_PINS : uint8_t
		{
		PIN_SX1276_NSS = 8,
		PIN_SX1276_NRESET = 4,
		PIN_SX1276_DIO0 = 7,
		PIN_SX1276_DIO1 = 6,
		PIN_SX1276_DIO2 = HalPinmap_t::UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_RX = HalPinmap_t::UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_TX_BOOST = HalPinmap_t::UNUSED_PIN,
		PIN_SX1276_ANT_SWITCH_TX_RFO = HalPinmap_t::UNUSED_PIN,
		PIN_VDD_BOOST_ENABLE = HalPinmap_t::UNUSED_PIN,
		};

	virtual void begin(void) override
		{
		digitalWrite(PIN_SX1276_NSS, 1);
		pinMode(PIN_SX1276_NSS, OUTPUT);
		}

	// virtual void end(void) override

	// virtual ostime_t setModuleActive(bool state) override

	};

static HalConfiguration_Feather32U4LoRa_t myConfig;

// Pin mapping for Adafruit Feather 32u4 LoRa, etc.
// Just like Feather M0 LoRa, but uses SPI at 1MHz; and that's only
// because MCCI doesn't have a test board; probably higher frequencies
// will work.

static const HalPinmap_t myPinmap =
        {
        .nss = HalConfiguration_Feather32U4LoRa_t::PIN_SX1276_NSS,      // chip select is D7
        .rxtx = HalConfiguration_Feather32U4LoRa_t::PIN_SX1276_ANT_SWITCH_RX, // RXTX is D29
        .rst = HalConfiguration_Feather32U4LoRa_t::PIN_SX1276_NRESET,   // NRESET is D8

        .dio = {HalConfiguration_Feather32U4LoRa_t::PIN_SX1276_DIO0,    // DIO0 (IRQ) is D25
                HalConfiguration_Feather32U4LoRa_t::PIN_SX1276_DIO1,    // DIO1 is D26
                HalConfiguration_Feather32U4LoRa_t::PIN_SX1276_DIO2,    // DIO2 is D27
               },
        .rxtx_rx_active = 0,
        .rssi_cal = 8,
        .spi_freq = 1000000,     /* 1MHz */
        .pConfig = &myConfig,
        };

const HalPinmap_t *GetPinmap_Feather32U4LoRa(void)
	{
	return &myPinmap;
	}

}; // namespace Arduino_LMIC
