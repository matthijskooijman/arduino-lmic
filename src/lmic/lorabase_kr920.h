/*
* Copyright (c) 2014-2016 IBM Corporation.
* All rights reserved.
*
* Copyright (c) 2017, 2019 MCCI Corporation
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

#ifndef _lorabase_kr920_h_
#define _lorabase_kr920_h_

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

/****************************************************************************\
|
| Basic definitions for KR920 (always in scope)
|
\****************************************************************************/

enum _dr_kr920_t {
        KR920_DR_SF12 = 0,      // DR0
        KR920_DR_SF11,          // DR1
        KR920_DR_SF10,          // DR2
        KR920_DR_SF9,           // DR3
        KR920_DR_SF8,           // DR4
        KR920_DR_SF7,           // DR5
        KR920_DR_NONE
};

// There is no dwell-time or duty-cycle limitation for IN
//
// max power: 30dBM
//
//                 freq                 datarates
enum {
        KR920_F1 = 922100000,      //   SF7-12 (DR0-5)
        KR920_F2 = 922300000,      //   SF7-12 (DR0-5)
        KR920_F3 = 922500000,      //   SF7-12 (DR0-5)
        KR920_FBCN = 923100000,    //   beacon/ping
        KR920_F14DBM = 922100000,  //   Allows 14 dBm (not 10) if >= this.
        KR920_FDOWN = 921900000,   //   RX2 downlink frequency
};
enum {
        KR920_FREQ_MIN = 920900000,
        KR920_FREQ_MAX = 923300000
};
enum {
        KR920_TX_EIRP_MAX_DBM = 14,     // 14 dBm for most
        KR920_TX_EIRP_MAX_DBM_LOW = 10, // 10 dBm for some
};
enum { DR_PAGE_KR920 = 0x10 * (LMIC_REGION_kr920 - 1) };

enum { KR920_LMIC_REGION_EIRP = 1 };    // region uses EIRP

enum { KR920_LBT_US = 128 };            // microseconds of LBT time.

enum { KR920_LBT_DB_MAX = -80 };        // maximum channel strength in dB; if TX
                                        // we measure more than this, we don't tx.

#endif /* _lorabase_in866_h_ */