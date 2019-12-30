/*
* Copyright (c) 2014-2016 IBM Corporation.
* Copyright (c) 2017, 2019 MCCI Corporation.
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

#define LMIC_DR_LEGACY 0

#include "lmic_bandplan.h"

#if defined(CFG_us915)
// ================================================================================
//
// BEG: US915 related stuff
//

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
        ILLEGAL_RPS,				// [-1]
        MAKERPS(SF10, BW125, CR_4_5, 0, 0),	// [0]
        MAKERPS(SF9 , BW125, CR_4_5, 0, 0),	// [1]
        MAKERPS(SF8 , BW125, CR_4_5, 0, 0),	// [2]
        MAKERPS(SF7 , BW125, CR_4_5, 0, 0),	// [3]
        MAKERPS(SF8 , BW500, CR_4_5, 0, 0),	// [4]
        ILLEGAL_RPS ,				// [5]
        ILLEGAL_RPS ,				// [6]
        ILLEGAL_RPS ,				// [7]
        MAKERPS(SF12, BW500, CR_4_5, 0, 0),	// [8]
        MAKERPS(SF11, BW500, CR_4_5, 0, 0),	// [9]
        MAKERPS(SF10, BW500, CR_4_5, 0, 0),	// [10]
        MAKERPS(SF9 , BW500, CR_4_5, 0, 0),	// [11]
        MAKERPS(SF8 , BW500, CR_4_5, 0, 0),	// [12]
        MAKERPS(SF7 , BW500, CR_4_5, 0, 0),	// [13]
        ILLEGAL_RPS				// [14]
};

static CONST_TABLE(u1_t, maxFrameLens)[] = {
        19+5, 61+5, 133+5, 250+5, 250+5, 0, 0,0,
        61+5, 133+5, 250+5, 250+5, 250+5, 250+5
        };

uint8_t LMICus915_maxFrameLen(uint8_t dr) {
        if (dr < LENOF_TABLE(maxFrameLens))
                return TABLE_GET_U1(maxFrameLens, dr);
        else
                return 0;
}

int8_t LMICus915_pow2dbm(uint8_t mcmd_ladr_p1) {
        if ((mcmd_ladr_p1 & MCMD_LinkADRReq_POW_MASK) >
            ((LMIC_LORAWAN_SPEC_VERSION < LMIC_LORAWAN_SPEC_VERSION_1_0_3)
                        ? US915_LinkAdrReq_POW_MAX_1_0_2
                        : US915_LinkAdrReq_POW_MAX_1_0_3))
                return -128;
        else
                return ((s1_t)(US915_TX_MAX_DBM - (((mcmd_ladr_p1)&MCMD_LinkADRReq_POW_MASK)<<1)));
}

static CONST_TABLE(ostime_t, DR2HSYM_osticks)[] = {
        us2osticksRound(128 << 5),  // DR_SF10   DR_SF12CR
        us2osticksRound(128 << 4),  // DR_SF9    DR_SF11CR
        us2osticksRound(128 << 3),  // DR_SF8    DR_SF10CR
        us2osticksRound(128 << 2),  // DR_SF7    DR_SF9CR
        us2osticksRound(128 << 1),  // DR_SF8C   DR_SF8CR
        us2osticksRound(128 << 0)   // ------    DR_SF7CR
};

ostime_t LMICus915_dr2hsym(uint8_t dr) {
        return TABLE_GET_OSTIME(DR2HSYM_osticks, (dr) & 7);  // map DR_SFnCR -> 0-6
}



u4_t LMICus915_convFreq(xref2cu1_t ptr) {
        u4_t freq = (os_rlsbf4(ptr - 1) >> 8) * 100;
        if (freq < US915_FREQ_MIN || freq > US915_FREQ_MAX)
                freq = 0;
        return freq;
}

bit_t LMIC_setupChannel(u1_t chidx, u4_t freq, u2_t drmap, s1_t band) {
        LMIC_API_PARAMETER(band);

        if (chidx < 72 || chidx >= 72 + MAX_XCHANNELS)
                return 0; // channels 0..71 are hardwired
        LMIC.xchFreq[chidx - 72] = freq;
        // TODO(tmm@mcci.com): don't use US SF directly, use something from the LMIC context or a static const
        LMIC.xchDrMap[chidx - 72] = drmap == 0 ? DR_RANGE_MAP(US915_DR_SF10, US915_DR_SF8C) : drmap;
        LMIC.channelMap[chidx >> 4] |= (1 << (chidx & 0xF));
        return 1;
}

bit_t LMIC_disableChannel(u1_t channel) {
        bit_t result = 0;
        if (channel < 72 + MAX_XCHANNELS) {
                if (ENABLED_CHANNEL(channel)) {
                        result = 1;
                        if (IS_CHANNEL_125khz(channel))
                                LMIC.activeChannels125khz--;
                        else if (IS_CHANNEL_500khz(channel))
                                LMIC.activeChannels500khz--;
                }
                LMIC.channelMap[channel >> 4] &= ~(1 << (channel & 0xF));
        }
        return result;
}

bit_t LMIC_enableChannel(u1_t channel) {
        bit_t result = 0;
        if (channel < 72 + MAX_XCHANNELS) {
                if (!ENABLED_CHANNEL(channel)) {
                        result = 1;
                        if (IS_CHANNEL_125khz(channel))
                                LMIC.activeChannels125khz++;
                        else if (IS_CHANNEL_500khz(channel))
                                LMIC.activeChannels500khz++;
                }
                LMIC.channelMap[channel >> 4] |= (1 << (channel & 0xF));
        }
        return result;
}

bit_t  LMIC_enableSubBand(u1_t band) {
        ASSERT(band < 8);
        u1_t start = band * 8;
        u1_t end = start + 8;
        bit_t result = 0;

        // enable all eight 125 kHz channels in this subband
        for (int channel = start; channel < end; ++channel)
                result |= LMIC_enableChannel(channel);

        // there's a single 500 kHz channel associated with
        // each group of 8 125 kHz channels. Enable it, too.
        result |= LMIC_enableChannel(64 + band);
        return result;
}

bit_t  LMIC_disableSubBand(u1_t band) {
        ASSERT(band < 8);
        u1_t start = band * 8;
        u1_t end = start + 8;
        bit_t result = 0;

        // disable all eight 125 kHz channels in this subband
        for (int channel = start; channel < end; ++channel)
                result |= LMIC_disableChannel(channel);

        // there's a single 500 kHz channel associated with
        // each group of 8 125 kHz channels. Disable it, too.
        result |= LMIC_disableChannel(64 + band);
        return result;
}

bit_t  LMIC_selectSubBand(u1_t band) {
        bit_t result = 0;

        ASSERT(band < 8);
        for (int b = 0; b<8; ++b) {
                if (band == b)
                        result |= LMIC_enableSubBand(b);
                else
                        result |= LMIC_disableSubBand(b);
        }
        return result;
}

void LMICus915_updateTx(ostime_t txbeg) {
        u1_t chnl = LMIC.txChnl;
        if (chnl < 64) {
                LMIC.freq = US915_125kHz_UPFBASE + chnl*US915_125kHz_UPFSTEP;
                if (LMIC.activeChannels125khz >= 50)
                        LMIC.txpow = 30;
                else
                        LMIC.txpow = 21;
        } else {
                // at 500kHz bandwidth, we're allowed more power.
                LMIC.txpow = 26;
                if (chnl < 64 + 8) {
                        LMIC.freq = US915_500kHz_UPFBASE + (chnl - 64)*US915_500kHz_UPFSTEP;
                }
                else {
                        ASSERT(chnl < 64 + 8 + MAX_XCHANNELS);
                        LMIC.freq = LMIC.xchFreq[chnl - 72];
                }
        }

        // Update global duty cycle stats
        if (LMIC.globalDutyRate != 0) {
                ostime_t airtime = calcAirTime(LMIC.rps, LMIC.dataLen);
                LMIC.globalDutyAvail = txbeg + (airtime << LMIC.globalDutyRate);
        }
}

#if !defined(DISABLE_BEACONS)
void LMICus915_setBcnRxParams(void) {
        LMIC.dataLen = 0;
        LMIC.freq = US915_500kHz_DNFBASE + LMIC.bcnChnl * US915_500kHz_DNFSTEP;
        LMIC.rps = setIh(setNocrc(dndr2rps((dr_t)DR_BCN), 1), LEN_BCN);
}
#endif // !DISABLE_BEACONS

// set the Rx1 dndr, rps.
void LMICus915_setRx1Params(void) {
    u1_t const txdr = LMIC.dndr;
    u1_t candidateDr;
    LMIC.freq = US915_500kHz_DNFBASE + (LMIC.txChnl & 0x7) * US915_500kHz_DNFSTEP;
    if ( /* TX datarate */txdr < LORAWAN_DR4)
            candidateDr = txdr + 10 - LMIC.rx1DrOffset;
    else
            candidateDr = LORAWAN_DR13 - LMIC.rx1DrOffset;

    if (candidateDr < LORAWAN_DR8)
            candidateDr = LORAWAN_DR8;
    else if (candidateDr > LORAWAN_DR13)
            candidateDr = LORAWAN_DR13;

    LMIC.dndr = candidateDr;
    LMIC.rps = dndr2rps(LMIC.dndr);
}

void LMICus915_initJoinLoop(void) {
    LMICuslike_initJoinLoop();

    // initialize the adrTxPower.
    LMIC.adrTxPow = 20; // dBm
}

//
// END: US915 related stuff
//
// ================================================================================
#endif
