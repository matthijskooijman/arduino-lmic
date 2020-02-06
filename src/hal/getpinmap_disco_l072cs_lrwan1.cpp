/*

Module:  getpinmap_disco_l072cz_lrwan1.cpp

Function:
        Arduino-LMIC C++ HAL pinmaps for the Discovery L072CZ LRWAN1

Copyright & License:
        See accompanying LICENSE file.

Author:
        Helium       February 2020

*/

#if defined(ARDUINO_DISCO_L072CZ_LRWAN1)

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

namespace Arduino_LMIC { 

    class HalConfiguration_Disco_L072cz_Lrwan1_t : public HalConfiguration_t
            {
    public:
            enum DIGITAL_PINS : uint8_t
                    {
                    PIN_SX1276_NSS = 37,
                    PIN_SX1276_NRESET = 33,
                    PIN_SX1276_DIO0 = 38,
                    PIN_SX1276_DIO1 = 39,
                    PIN_SX1276_DIO2 = 40,
                    PIN_SX1276_RXTX = 21,
                    };

        virtual bool queryUsingTcxo(void) override { return false; };
            };
    // save some typing by bringing the pin numbers into scope
    static HalConfiguration_Disco_L072cz_Lrwan1_t myConfig;

    static const HalPinmap_t myPinmap =
            {
            .nss = HalConfiguration_Disco_L072cz_Lrwan1_t::PIN_SX1276_NSS,
            .rxtx = HalConfiguration_Disco_L072cz_Lrwan1_t::PIN_SX1276_RXTX, 
            .rst = HalConfiguration_Disco_L072cz_Lrwan1_t::PIN_SX1276_NRESET,

            .dio = {HalConfiguration_Disco_L072cz_Lrwan1_t::PIN_SX1276_DIO0, 
                    HalConfiguration_Disco_L072cz_Lrwan1_t::PIN_SX1276_DIO1, 
                    HalConfiguration_Disco_L072cz_Lrwan1_t::PIN_SX1276_DIO2, 
                },
            .rxtx_rx_active = 1,
            .rssi_cal = 10,
            .spi_freq = 8000000,     /* 8MHz */
            .pConfig = &myConfig
            };

    const HalPinmap_t *GetPinmap_Disco_L072cz_Lrwan1(void)
            {
            return &myPinmap;
            }

}; // namespace Arduino_LMIC

#endif /* defined(ARDUINO_DISCO_L072CZ_LRWAN1) */
