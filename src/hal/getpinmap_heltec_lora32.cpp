/*

Module:  getpinmap_heltec_lora32.cpp

Function:
        Arduino-LMIC C++ HAL pinmap for Heltec WiFi LoRa 32 (V1 & V2) and Heltec Wireless Stick

Copyright & License:
        See accompanying LICENSE file.

Author:
        Manuel Bleichenbacher, manuel.bleichenbacher@gmail.com   October 2019

*/

#include <arduino_lmic_hal_boards.h>
#include <Arduino.h>

#include "../lmic/oslmic.h"

// Note: The pin constants SS, RST_LoRa and DIOx are defined in pins_arduino.h
// (board-specific variant in Arduino core). Even if it won't be used, this
// file needs to compile for all other variants as well.
#if !defined(ARDUINO_HELTEC_WIFI_LORA_32) && !defined(ARDUINO_HELTEC_WIFI_LORA_32_V2) && !defined(ARDUINO_HELTEC_WIRELESS_STICK)
#undef SS
#undef RST_LoRa
#undef DIO0
#undef DIO1
#undef DIO2
#define SS        HalPinmap_t::UNUSED_PIN
#define RST_LoRa  HalPinmap_t::UNUSED_PIN
#define DIO0      HalPinmap_t::UNUSED_PIN
#define DIO1      HalPinmap_t::UNUSED_PIN
#define DIO2      HalPinmap_t::UNUSED_PIN
#endif


namespace Arduino_LMIC
{

class HalConfiguration_heltec_lora32 : public HalConfiguration_t
{
public:
    enum DIGITAL_PINS : uint8_t
    {
        PIN_SX1276_NSS = SS,
        PIN_SX1276_NRESET = RST_LoRa,
        PIN_SX1276_DIO0 = DIO0,
        PIN_SX1276_DIO1 = DIO1,
        PIN_SX1276_DIO2 = DIO2,
        PIN_SX1276_ANT_SWITCH_RX = HalPinmap_t::UNUSED_PIN,
        PIN_SX1276_ANT_SWITCH_TX_BOOST = HalPinmap_t::UNUSED_PIN,
        PIN_SX1276_ANT_SWITCH_TX_RFO = HalPinmap_t::UNUSED_PIN,
        PIN_VDD_BOOST_ENABLE = HalPinmap_t::UNUSED_PIN,
    };
};

static HalConfiguration_heltec_lora32 myConfig;

static const HalPinmap_t myPinmap =
    {
        .nss = HalConfiguration_heltec_lora32::PIN_SX1276_NSS,
        .rxtx = HalConfiguration_heltec_lora32::PIN_SX1276_ANT_SWITCH_RX,
        .rst = HalConfiguration_heltec_lora32::PIN_SX1276_NRESET,
        .dio = {
            HalConfiguration_heltec_lora32::PIN_SX1276_DIO0,
            HalConfiguration_heltec_lora32::PIN_SX1276_DIO1,
            HalConfiguration_heltec_lora32::PIN_SX1276_DIO2,
        },
        .rxtx_rx_active = 0,
        .rssi_cal = 10,
        .spi_freq = 8000000, /* 8MHz */
        .pConfig = &myConfig};

const HalPinmap_t *GetPinmap_heltec_lora32(void)
{
    return &myPinmap;
}

}; // namespace Arduino_LMIC
