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

#if defined(CFG_au915)
// ================================================================================
//
// BEG: AU915 related stuff
//

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
        ILLEGAL_RPS,                            // [-1]
        MAKERPS(SF12, BW125, CR_4_5, 0, 0),     // [0]
        MAKERPS(SF11, BW125, CR_4_5, 0, 0),     // [1]
        MAKERPS(SF10, BW125, CR_4_5, 0, 0),     // [2]
        MAKERPS(SF9 , BW125, CR_4_5, 0, 0),     // [3]
        MAKERPS(SF8 , BW125, CR_4_5, 0, 0),     // [4]
        MAKERPS(SF7 , BW125, CR_4_5, 0, 0),     // [5]
        MAKERPS(SF8 , BW500, CR_4_5, 0, 0),     // [6]
        ILLEGAL_RPS ,                           // [7]
        MAKERPS(SF12, BW500, CR_4_5, 0, 0),     // [8]
        MAKERPS(SF11, BW500, CR_4_5, 0, 0),     // [9]
        MAKERPS(SF10, BW500, CR_4_5, 0, 0),     // [10]
        MAKERPS(SF9 , BW500, CR_4_5, 0, 0),     // [11]
        MAKERPS(SF8 , BW500, CR_4_5, 0, 0),     // [12]
        MAKERPS(SF7 , BW500, CR_4_5, 0, 0),     // [13]
        ILLEGAL_RPS
};

static CONST_TABLE(u1_t, maxFrameLens_dwell0)[] = {
        59+5,  59+5,  59+5, 123+5, 250+5, 250+5, 250+5, 0,
        61+5, 137+5, 250+5, 250+5, 250+5, 250+5 };

static CONST_TABLE(u1_t, maxFrameLens_dwell1)[] = {
        0,        0,  19+5,  61+5, 133+5, 250+5, 250+5, 0,
        61+5, 137+5, 250+5, 250+5, 250+5, 250+5 };

static bit_t
LMICau915_getUplinkDwellBit() {
        // if uninitialized, return default.
        if (LMIC.txParam == 0xFF) {
                return AU915_INITIAL_TxParam_UplinkDwellTime;
        }
        return (LMIC.txParam & MCMD_TxParam_TxDWELL_MASK) != 0;
}

uint8_t LMICau915_maxFrameLen(uint8_t dr) {
        if (LMICau915_getUplinkDwellBit()) {
                if (dr < LENOF_TABLE(maxFrameLens_dwell0))
                        return TABLE_GET_U1(maxFrameLens_dwell0, dr);
                else
                        return 0;
        } else {
                if (dr < LENOF_TABLE(maxFrameLens_dwell1))
                        return TABLE_GET_U1(maxFrameLens_dwell1, dr);
                else
                        return 0;
        }
}

// from LoRaWAN 5.8: mapping from txParam to MaxEIRP
static CONST_TABLE(s1_t, TXMAXEIRP)[16] = {
	8, 10, 12, 13, 14, 16, 18, 20, 21, 24, 26, 27, 29, 30, 33, 36
};

static int8_t LMICau915_getMaxEIRP(uint8_t mcmd_txparam) {
        // if uninitialized, return default.
	if (mcmd_txparam == 0xFF)
		return AU915_TX_EIRP_MAX_DBM;
	else
		return TABLE_GET_S1(
			TXMAXEIRP,
			(mcmd_txparam & MCMD_TxParam_MaxEIRP_MASK) >>
				MCMD_TxParam_MaxEIRP_SHIFT
			);
}

int8_t LMICau915_pow2dbm(uint8_t mcmd_ladr_p1) {
        if ((mcmd_ladr_p1 & MCMD_LinkADRReq_POW_MASK) == MCMD_LinkADRReq_POW_MASK)
                return -128;
        else    {
                return ((s1_t)(LMICau915_getMaxEIRP(LMIC.txParam) - (((mcmd_ladr_p1)&MCMD_LinkADRReq_POW_MASK)<<1)));
        }
}

static CONST_TABLE(ostime_t, DR2HSYM_osticks)[] = {
        us2osticksRound(128 << 7),  // DR_SF12
        us2osticksRound(128 << 6),  // DR_SF11
        us2osticksRound(128 << 5),  // DR_SF10
        us2osticksRound(128 << 4),  // DR_SF9
        us2osticksRound(128 << 3),  // DR_SF8
        us2osticksRound(128 << 2),  // DR_SF7
        us2osticksRound(128 << 1),  // DR_SF8C
        us2osticksRound(128 << 0),  // ------
        us2osticksRound(128 << 5),  // DR_SF12CR
        us2osticksRound(128 << 4),  // DR_SF11CR
        us2osticksRound(128 << 3),  // DR_SF10CR
        us2osticksRound(128 << 2),  // DR_SF9CR
        us2osticksRound(128 << 1),  // DR_SF8CR
        us2osticksRound(128 << 0),  // DR_SF7CR
};

// get ostime for symbols based on datarate. This is not like us915,
// becuase the times don't match between the upper half and lower half
// of the table.
ostime_t LMICau915_dr2hsym(uint8_t dr) {
        return TABLE_GET_OSTIME(DR2HSYM_osticks, dr);
}



u4_t LMICau915_convFreq(xref2cu1_t ptr) {
        u4_t freq = (os_rlsbf4(ptr - 1) >> 8) * 100;
        if (freq < AU915_FREQ_MIN || freq > AU915_FREQ_MAX)
                freq = 0;
        return freq;
}

// au915: no support for xchannels.
bit_t LMIC_setupChannel(u1_t chidx, u4_t freq, u2_t drmap, s1_t band) {
        LMIC_API_PARAMETER(chidx);
        LMIC_API_PARAMETER(freq);
        LMIC_API_PARAMETER(drmap);
        LMIC_API_PARAMETER(band);

        return 0; // all channels are hardwired.
}

bit_t LMIC_disableChannel(u1_t channel) {
        bit_t result = 0;
        if (channel < 72) {
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
        if (channel < 72) {
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

bit_t LMIC_enableSubBand(u1_t band) {
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

bit_t LMIC_disableSubBand(u1_t band) {
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

bit_t LMIC_selectSubBand(u1_t band) {
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

void LMICau915_updateTx(ostime_t txbeg) {
        u1_t chnl = LMIC.txChnl;
        LMIC.txpow = LMICau915_getMaxEIRP(LMIC.txParam);
        if (chnl < 64) {
                LMIC.freq = AU915_125kHz_UPFBASE + chnl*AU915_125kHz_UPFSTEP;
        } else {
                ASSERT(chnl < 64 + 8);
                LMIC.freq = AU915_500kHz_UPFBASE + (chnl - 64)*AU915_500kHz_UPFSTEP;
        }

        // Update global duty cycle stat and deal with dwell time.
        u4_t dwellDelay;
        u4_t globalDutyDelay;
        dwellDelay = globalDutyDelay = 0;

        if (LMIC.globalDutyRate != 0) {
                ostime_t airtime = calcAirTime(LMIC.rps, LMIC.dataLen);
                globalDutyDelay = txbeg + (airtime << LMIC.globalDutyRate);
        }
        if (LMICau915_getUplinkDwellBit(LMIC.txParam)) {
                dwellDelay = AU915_UPLINK_DWELL_TIME_osticks;
        }
        if (dwellDelay > globalDutyDelay) {
                globalDutyDelay = dwellDelay;
        }
        if (globalDutyDelay != 0) {
                LMIC.globalDutyAvail = txbeg + globalDutyDelay;
        }
}

#if !defined(DISABLE_BEACONS)
void LMICau915_setBcnRxParams(void) {
        LMIC.dataLen = 0;
        LMIC.freq = AU915_500kHz_DNFBASE + LMIC.bcnChnl * AU915_500kHz_DNFSTEP;
        LMIC.rps = setIh(setNocrc(dndr2rps((dr_t)DR_BCN), 1), LEN_BCN);
}
#endif // !DISABLE_BEACONS

// set the Rx1 dndr, rps.
void LMICau915_setRx1Params(void) {
        u1_t const txdr = LMIC.dndr;
        u1_t candidateDr;
        LMIC.freq = AU915_500kHz_DNFBASE + (LMIC.txChnl & 0x7) * AU915_500kHz_DNFSTEP;
        if ( /* TX datarate */txdr < AU915_DR_SF8C)
                candidateDr = txdr + 8 - LMIC.rx1DrOffset;
        else
                candidateDr = AU915_DR_SF7CR;

        if (candidateDr < LORAWAN_DR8)
                candidateDr = LORAWAN_DR8;
        else if (candidateDr > LORAWAN_DR13)
                candidateDr = LORAWAN_DR13;

        LMIC.dndr = candidateDr;
        LMIC.rps = dndr2rps(LMIC.dndr);
}

void LMICau915_initJoinLoop(void) {
        // LMIC.txParam is set to 0xFF by the central code at init time.
        LMICuslike_initJoinLoop();

        // initialize the adrTxPower.
        LMIC.adrTxPow = LMICau915_getMaxEIRP(LMIC.txParam); // dBm

}

//
// END: AU915 related stuff
//
// ================================================================================
#endif
