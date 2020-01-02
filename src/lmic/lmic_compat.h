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

// Frequency plan symbols
# define AU921_DR_SF12	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF12
# define AU921_DR_SF11	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF11
# define AU921_DR_SF10	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF10
# define AU921_DR_SF9	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF9
# define AU921_DR_SF8	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF8
# define AU921_DR_SF7	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF7
# define AU921_DR_SF8C	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF8C
# define AU921_DR_NONE	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_NONE
# define AU921_DR_SF12CR LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF12CR
# define AU921_DR_SF11CR LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF11CR
# define AU921_DR_SF10CR LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF10CR
# define AU921_DR_SF9CR	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF9CR
# define AU921_DR_SF8CR	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF8CR
# define AU921_DR_SF7CR	LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_DR_SF7CR
# define AU921_125kHz_UPFBASE   LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_125kHz_UPFBASE
# define AU921_125kHz_UPFSTEP   LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_125kHz_UPFSTEP
# define AU921_500kHz_UPFBASE   LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_500kHz_UPFBASE
# define AU921_500kHz_UPFSTEP   LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_500kHz_UPFSTEP
# define AU921_500kHz_DNFBASE   LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_500kHz_DNFBASE
# define AU921_500kHz_DNFSTEP   LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_500kHz_DNFSTEP
# define AU921_FREQ_MIN         LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_FREQ_MIN
# define AU921_FREQ_MAX         LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_FREQ_MAX
# define AU921_TX_EIRP_MAX_DBM  LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_TX_EIRP_MAX_DBM
# define AU921_INITIAL_TxParam_UplinkDwellTime      LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_INITIAL_TxParam_UplinkDwellTime
# define AU921_UPLINK_DWELL_TIME_osticks            LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_UPLINK_DWELL_TIME_osticks
# define DR_PAGE_AU921          LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") DR_PAGE_AU915
# define AU921_LMIC_REGION_EIRP LMIC_DEPRECATE(GCC warning "A921 symbols are deprecated EOL V5, use AU915") AU915_LMIC_REGION_EIRP
#endif

#ifdef __cplusplus
}
#endif

#endif /* _lmic_compat_h_ */
