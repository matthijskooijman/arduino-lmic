/*

Module:  getconfig_ttgo_lora32_v1.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for TTGO ESP32 OLED V1

Copyright & License:
        See accompanying LICENSE file.

Author:
        German Martin, gmag11@gmail.com   June 2019

*/

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

#define LORA_DIO0 26
#define LORA_DIO1 33
#define LORA_DIO2 32

namespace Arduino_LMIC {

class HalConfiguration_ttgo_lora32_v1 : public HalConfiguration_t
	{
public:
	enum DIGITAL_PINS : uint8_t
		{
		PIN_SX1276_NSS = 18,
		PIN_SX1276_NRESET = 14,
		PIN_SX1276_DIO0 = LORA_DIO0,
		PIN_SX1276_DIO1 = LORA_DIO1,
		PIN_SX1276_DIO2 = LORA_DIO2,
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

static HalConfiguration_ttgo_lora32_v1 myConfig;

static const HalPinmap_t myPinmap =
        {
        .nss = HalConfiguration_ttgo_lora32_v1::PIN_SX1276_NSS,      // chip select is D7
        .rxtx = HalConfiguration_ttgo_lora32_v1::PIN_SX1276_ANT_SWITCH_RX, // RXTX is D29
        .rst = HalConfiguration_ttgo_lora32_v1::PIN_SX1276_NRESET,   // NRESET is D8

        .dio = {HalConfiguration_ttgo_lora32_v1::PIN_SX1276_DIO0,    // DIO0 (IRQ) is D25
				HalConfiguration_ttgo_lora32_v1::PIN_SX1276_DIO1,    // DIO1 is D26
				HalConfiguration_ttgo_lora32_v1::PIN_SX1276_DIO2,    // DIO2 is D27
               },
        .rxtx_rx_active = 0,
        .rssi_cal = 10,
        .spi_freq = 8000000,     /* 8MHz */
        .pConfig = &myConfig
        };

const HalPinmap_t * GetPinmap_ttgo_lora32_v1 (void)
	{
	return &myPinmap;
	}

}; // namespace Arduino_LMIC
