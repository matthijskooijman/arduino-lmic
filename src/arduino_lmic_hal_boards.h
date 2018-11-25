/*

Module:  arduino_lmic_hal_boards.h

Function:
        Arduino-LMIC C++ HAL pinmaps for various boards

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#pragma once

#ifndef _arduino_lmic_hal_boards_h_
# define _arduino_lmic_hal_boards_h_

#include "arduino_lmic_hal_configuration.h"

namespace Arduino_LMIC {

const HalPinmap_t *GetPinmap_FeatherM0LoRa();
const HalPinmap_t *GetPinmap_Feather32U4LoRa();

const HalPinmap_t *GetPinmap_Catena4420();
const HalPinmap_t *GetPinmap_Catena4551();
const HalPinmap_t *GetPinmap_Catena4610();
const HalPinmap_t *GetPinmap_Catena4610();
const HalPinmap_t *GetPinmap_Catena4611();
const HalPinmap_t *GetPinmap_Catena4612();
const HalPinmap_t *GetPinmap_Catena4801();

const HalPinmap_t *GetPinmap_ThisBoard();

}; /* namespace Arduino_LIMC */

#endif /* _arduino_lmic_hal_boards_h_ */
