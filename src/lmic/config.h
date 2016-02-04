#ifndef _lmic_config_h_
#define _lmic_config_h_

// In the original LMIC code, these config values were defined on the
// gcc commandline. Since Arduino does not allow easily modifying the
// compiler commandline, use this file instead.

#define CFG_eu868 1
//#define CFG_us915 1
#define CFG_sx1272_radio 1
//#define CFG_sx1276_radio 1

// 16 μs per tick
// LMIC requires ticks to be 15.5μs - 100 μs long
#define US_PER_OSTICK_EXPONENT 4
#define US_PER_OSTICK (1 << US_PER_OSTICK_EXPONENT)
#define OSTICKS_PER_SEC (1000000 / US_PER_OSTICK)

// hal.cpp sets up stdio so that a plain "printf" call prints to the
// given serial port (or any other Print object). Any assertion errors
// are also printed to the same place. Comment this define to prevent
// this printf setup, as well as making failed assertions silent.
#define LMIC_PRINTF_TO Serial

// Uncomment this to disable all code related to joining
//#define DISABLE_JOIN
// Uncomment this to disable all code related to ping
//#define DISABLE_PING
// Uncomment this to disable all code related to beacon tracking.
// Requires ping to be disabled too
//#define DISABLE_BEACONS

#endif // _lmic_config_h_
