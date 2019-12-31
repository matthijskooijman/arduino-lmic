/*
* Copyright (c) 2014-2016 IBM Corporation.
* All rights reserved.
*
* Copyright (c) 2017 MCCI Corporation
* All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of the <organization> nor the
*    names of its contributors may be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _lorabase_au915_h_
#define _lorabase_au915_h_

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

/****************************************************************************\
|
| Basic definitions for AU 915 (always in scope)
|
\****************************************************************************/

// Frequency plan for AU 915 MHz
enum _dr_au915_t {
        AU915_DR_SF12 = 0,
        AU915_DR_SF11,
        AU915_DR_SF10,
        AU915_DR_SF9,
        AU915_DR_SF8,
        AU915_DR_SF7,
        AU915_DR_SF8C,
        AU915_DR_NONE,
        // Devices behind a router:
        AU915_DR_SF12CR = 8,
        AU915_DR_SF11CR,
        AU915_DR_SF10CR,
        AU915_DR_SF9CR,
        AU915_DR_SF8CR,
        AU915_DR_SF7CR
};

// Default frequency plan for AU 915MHz
enum {
        AU915_125kHz_UPFBASE = 915200000,
        AU915_125kHz_UPFSTEP = 200000,
        AU915_500kHz_UPFBASE = 915900000,
        AU915_500kHz_UPFSTEP = 1600000,
        AU915_500kHz_DNFBASE = 923300000,
        AU915_500kHz_DNFSTEP = 600000
};
enum {
        AU915_FREQ_MIN = 915000000,
        AU915_FREQ_MAX = 928000000
};
enum {
        AU915_TX_EIRP_MAX_DBM = 30      // 30 dBm
};
enum {
        // initial value of UplinkDwellTime before TxParamSetupReq received.
        AU915_INITIAL_TxParam_UplinkDwellTime = 1,
        AU915_UPLINK_DWELL_TIME_osticks = sec2osticks(20),
};

enum { DR_PAGE_AU915 = 0x10 * (LMIC_REGION_au915 - 1) };

enum { AU915_LMIC_REGION_EIRP = 1 };         // region uses EIRP

#endif /* _lorabase_au915_h_ */