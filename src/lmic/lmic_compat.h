/*

Module:  lmic_compat.h

Function:
	Symbols that are defined for backward compatibility

Copyright notice and license info:
	See LICENSE file accompanying this project.

Author:
	Terry Moore, MCCI Corporation	January 2020

Description:
	This include file centralizes backwards compatibility
	definitions. The idea is to centralize the decision,
	so it's clear as to what's deprecated.

*/

#ifndef _lmic_compat_h_	/* prevent multiple includes */
#define _lmic_compat_h_

#ifdef __cplusplus
extern "C"{
#endif

#ifndef ARDUINO_LMIC_VERSION
# error "This file is normally included from lmic.h, not stand alone"
#endif

#define LMIC_DEPRECATE(m)	_Pragma(#m)

#if ! defined(LMIC_REGION_au921) && ARDUINO_LMIC_VERSION < ARDUINO_LMIC_VERSION_CALC(5,0,0,0) 
# define LMIC_REGION_au921	LMIC_DEPRECATE(GCC warning "LMIC_REGION_au921 is deprecated, EOL at V5, use LMIC_REGION_au915") \
				LMIC_REGION_au915
#endif

#ifdef __cplusplus
}
#endif

#endif /* _lmic_compat_h_ */
