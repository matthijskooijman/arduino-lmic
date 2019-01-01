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

namespace Arduino_LMIC {

const HalPinmap_t *GetPinmap_ThisBoard(void)
        {
#if defined(ARDUINO_SAMD_FEATHER_M0)
# if defined(ARDUINO_MCCI_CATENA_4420)
        // this uses a radiowing and an odd configuration
        return GetPinmap_Catena4420();
# else
        // others use Feather M0 LoRa
        return GetPinmap_FeatherM0LoRa();
# endif
#elif defined(ARDUINO_AVR_FEATHER32U4)
        return GetPinmap_Feather32U4LoRa();
#elif defined(ARDUINO_MCCI_CATENA_4551) || \
      /* legacy names */ \
      defined(ARDUINO_CATENA_4551)
        return GetPinmap_Catena4551();
#elif defined(ARDUINO_MCCI_CATENA_4610)
        return GetPinmap_Catena4610();
#elif defined(ARDUINO_MCCI_CATENA_4611) || \
      /* legacy names */ \
      defined(ARDUINO_CATENA_4611)
        return GetPinmap_Catena4611();
#elif defined(ARDUINO_MCCI_CATENA_4612) || \
      /* legacy names */ \
      defined(ARDUINO_CATENA_4612)
        return GetPinmap_Catena4612();
#elif defined(ARDUINO_MCCI_CATENA_4801)
        return GetPinmap_Catena4801();
#elif defined(PINNOCHIO_SCOUT)
        return GetPinmap_PinnochioScount();
#else
        #pragma message("Board not supported -- use an explicit pinmap")
        return nullptr;
#endif
        }

}; // namespace Arduino_LMIC

