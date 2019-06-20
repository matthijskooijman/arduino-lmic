/*

Module:  arduino_lmic_hal_boards.h

Function:
	Arduino-LMIC C++ HAL pinmaps for various boards

Copyright & License:
	See accompanying LICENSE file.

Author:
	Terry Moore, MCCI	November 2018

Revision history:
	Mon Nov 26 2018 15:06:30  tmm
	Module created.

	Wed Jun 19 2019 12:29:35  lakshmipriyan
	Added support for catena 4617 & catena 4618.

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
const HalPinmap_t *GetPinmap_Catena4617();
const HalPinmap_t *GetPinmap_Catena4618();
const HalPinmap_t *GetPinmap_Catena4801();

const HalPinmap_t *GetPinmap_ThisBoard();

}; /* namespace Arduino_LIMC */

#endif /* _arduino_lmic_hal_boards_h_ */
