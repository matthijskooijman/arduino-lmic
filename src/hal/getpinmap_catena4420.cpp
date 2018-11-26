/*

Module:  getconfig_catena4420.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for MCCI Catena 4420

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

namespace Arduino_LMIC {

class HalConfiguration_Catena4420_t : public HalConfiguration_t
	{
public:
	enum DIGITAL_PINS : uint8_t
		{
		PIN_SX1276_NSS = 6,
		PIN_SX1276_NRESET = 5,
		PIN_SX1276_DIO0 = 12,    // pin assignment for DIO0 (aka IRQ)
		PIN_SX1276_DIO1 = 11,
		PIN_SX1276_DIO2 = 10,
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

static HalConfiguration_Catena4420_t myConfig;

static const HalPinmap_t myPinmap =
        {
        .nss = HalConfiguration_Catena4420_t::PIN_SX1276_NSS,      // chip select is D7
        .rxtx = HalConfiguration_Catena4420_t::PIN_SX1276_ANT_SWITCH_RX, // RXTX is D29
        .rst = HalConfiguration_Catena4420_t::PIN_SX1276_NRESET,   // NRESET is D8

        .dio = {HalConfiguration_Catena4420_t::PIN_SX1276_DIO0,    // DIO0 (IRQ) is D25
                HalConfiguration_Catena4420_t::PIN_SX1276_DIO1,    // DIO1 is D26
                HalConfiguration_Catena4420_t::PIN_SX1276_DIO2,    // DIO2 is D27
               },
        .rxtx_rx_active = 0,
        .rssi_cal = 10,
        .spi_freq = 8000000,     /* 8MHz */
        .pConfig = &myConfig
        };

const HalPinmap_t *GetPinmap_Catena4420(void)
	{
	return &myPinmap;
	}

} // namespace Arduino_LMIC;