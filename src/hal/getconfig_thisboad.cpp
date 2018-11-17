/*

Module:  getconfig_thisboard.cpp

Function:
        Return a suitable LMIC config for this board.

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

*/

#include <arduino_lmic_hal_boards.h>

using namespace Arduino_LMIC {

const HalConfiguration_t *GetConfig_ThisBoard(void)
	{
#if defined(ARDUINO_SAMD_FEATHER_M0)
	return GetConfig_FeatherM0Lora();
#elif defined(ARDUINO_AVR_FEATHER32U4)
	return GetConfig_Feather32U4LoRa();
#elif defined(ARDUINO_CATENA_4551)
	return GetConfig_Catena4551();
#elif defined(ARDUINO_CATENA_4610)
#elif defined(ARDUINO_CATENA_4611) || defined(ARDUINO_CATENA_4612)
	return GetConfig_Catena4612();
#elif defined(ARDUINO_CATENA_4801)
	return GetConfig_Catena4612();
#else
	#warning Board not supported
	return nullptr;
#endif
	}
