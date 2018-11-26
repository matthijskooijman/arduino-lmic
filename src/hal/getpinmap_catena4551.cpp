/*

Module:  getconfig_catena4551.cpp

Function:
        Arduino-LMIC C++ HAL pinmaps for various boards

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#if defined(ARDUINO_MCCI_CATENA_4551) || \
    /* legacy names */ \
    defined(ARDUINO_CATENA_4551)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

namespace Arduino_LMIC {

class HalConfiguration_Catena4551_t : public HalConfiguration_t
        {
public:
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

	virtual void begin(void) override
		{
		digitalWrite(PIN_SX1276_NSS, 1);
		pinMode(PIN_SX1276_NSS, OUTPUT);
		}

	// virtual void end(void) override

	// On the 4551, we can't control the TCXO; it's always on.
	// So we use the default.
	// virtual ostime_t setModuleActive(bool state) override
	virtual bool queryUsingTcxo(void) override { return true; };
        };

// save some typing by bringing the pin numbers into scope
static HalConfiguration_Catena4551_t myConfig;

static const HalPinmap_t myPinmap =
        {
        .nss = HalConfiguration_Catena4551_t::PIN_SX1276_NSS,      // chip select is D7
        .rxtx = HalConfiguration_Catena4551_t::PIN_SX1276_ANT_SWITCH_RX, // RXTX is D29
        .rst = HalConfiguration_Catena4551_t::PIN_SX1276_NRESET,   // NRESET is D8

        .dio = {HalConfiguration_Catena4551_t::PIN_SX1276_DIO0,    // DIO0 (IRQ) is D25
                HalConfiguration_Catena4551_t::PIN_SX1276_DIO1,    // DIO1 is D26
                HalConfiguration_Catena4551_t::PIN_SX1276_DIO2,    // DIO2 is D27
               },
        .rxtx_rx_active = 1,
        .rssi_cal = 10,
        .spi_freq = 8000000,     /* 8MHz */
        .pConfig = &myConfig,
        };

const HalPinmap_t *GetPinmap_Catena4551(void)
        {
        return &myPinmap;
        }

}; // namespace Arduino_LMIC

#endif /* defined(ARDUINO_MCCI_CATENA_4551) */
