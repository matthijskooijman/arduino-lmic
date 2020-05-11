/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 * Copyright (c) 2016-2019 MCCI Corporation.
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

//! \file
#define LMIC_DR_LEGACY 0
#include "lmic_bandplan.h"

#if defined(DISABLE_BEACONS) && !defined(DISABLE_PING)
#error Ping needs beacon tracking
#endif

DEFINE_LMIC;

// Fwd decls.
static void reportEventNoUpdate(ev_t);
static void reportEventAndUpdate(ev_t);
static void engineUpdate(void);
static bit_t processJoinAccept_badframe(void);
static bit_t processJoinAccept_nojoinframe(void);


#if !defined(DISABLE_BEACONS)
static void startScan (void);
#endif

// set the txrxFlags, with debugging
static inline void initTxrxFlags(const char *func, u1_t mask) {
	LMIC_DEBUG2_PARAMETER(func);

#if LMIC_DEBUG_LEVEL > 1
	LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": %s txrxFlags %#02x --> %02x\n", os_getTime(), func, LMIC.txrxFlags, mask);
#endif
	LMIC.txrxFlags = mask;
}

// or the txrxFlags, with debugging
static inline void orTxrxFlags(const char *func, u1_t mask) {
	initTxrxFlags(func, LMIC.txrxFlags | mask);
}



// ================================================================================
// BEG OS - default implementations for certain OS suport functions

#if !defined(HAS_os_calls)

#if !defined(os_rlsbf2)
u2_t os_rlsbf2 (xref2cu1_t buf) {
    return (u2_t)((u2_t)buf[0] | ((u2_t)buf[1]<<8));
}
#endif

#if !defined(os_rlsbf4)
u4_t os_rlsbf4 (xref2cu1_t buf) {
    return (u4_t)((u4_t)buf[0] | ((u4_t)buf[1]<<8) | ((u4_t)buf[2]<<16) | ((u4_t)buf[3]<<24));
}
#endif


#if !defined(os_rmsbf4)
u4_t os_rmsbf4 (xref2cu1_t buf) {
    return (u4_t)((u4_t)buf[3] | ((u4_t)buf[2]<<8) | ((u4_t)buf[1]<<16) | ((u4_t)buf[0]<<24));
}
#endif


#if !defined(os_wlsbf2)
void os_wlsbf2 (xref2u1_t buf, u2_t v) {
    buf[0] = v;
    buf[1] = v>>8;
}
#endif

#if !defined(os_wlsbf4)
void os_wlsbf4 (xref2u1_t buf, u4_t v) {
    buf[0] = v;
    buf[1] = v>>8;
    buf[2] = v>>16;
    buf[3] = v>>24;
}
#endif

#if !defined(os_wmsbf4)
void os_wmsbf4 (xref2u1_t buf, u4_t v) {
    buf[3] = v;
    buf[2] = v>>8;
    buf[1] = v>>16;
    buf[0] = v>>24;
}
#endif

#if !defined(os_getBattLevel)
u1_t os_getBattLevel (void) {
    return MCMD_DEVS_BATT_NOINFO;
}
#endif

#if !defined(os_crc16)
// New CRC-16 CCITT(XMODEM) checksum for beacons:
u2_t os_crc16 (xref2cu1_t data, uint len) {
    u2_t remainder = 0;
    u2_t polynomial = 0x1021;
    for( uint i = 0; i < len; i++ ) {
        remainder ^= data[i] << 8;
        for( u1_t bit = 8; bit > 0; bit--) {
            if( (remainder & 0x8000) )
                remainder = (remainder << 1) ^ polynomial;
            else
                remainder <<= 1;
        }
    }
    return remainder;
}
#endif

#endif // !HAS_os_calls

// END OS - default implementations for certain OS suport functions
// ================================================================================

// ================================================================================
// BEG AES

static void micB0 (u4_t devaddr, u4_t seqno, int dndir, int len) {
    os_clearMem(AESaux,16);
    AESaux[0]  = 0x49;
    AESaux[5]  = dndir?1:0;
    AESaux[15] = len;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
}


static int aes_verifyMic (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t pdu, int len) {
    micB0(devaddr, seqno, dndir, len);
    os_copyMem(AESkey,key,16);
    return os_aes(AES_MIC, pdu, len) == os_rmsbf4(pdu+len);
}


static void aes_appendMic (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t pdu, int len) {
    micB0(devaddr, seqno, dndir, len);
    os_copyMem(AESkey,key,16);
    // MSB because of internal structure of AES
    os_wmsbf4(pdu+len, os_aes(AES_MIC, pdu, len));
}


static void aes_appendMic0 (xref2u1_t pdu, int len) {
    os_getDevKey(AESkey);
    os_wmsbf4(pdu+len, os_aes(AES_MIC|AES_MICNOAUX, pdu, len));  // MSB because of internal structure of AES
}


static int aes_verifyMic0 (xref2u1_t pdu, int len) {
    os_getDevKey(AESkey);
    return os_aes(AES_MIC|AES_MICNOAUX, pdu, len) == os_rmsbf4(pdu+len);
}


static void aes_encrypt (xref2u1_t pdu, int len) {
    os_getDevKey(AESkey);
    os_aes(AES_ENC, pdu, len);
}


static void aes_cipher (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t payload, int len) {
    if( len <= 0 )
        return;
    os_clearMem(AESaux, 16);
    AESaux[0] = AESaux[15] = 1; // mode=cipher / dir=down / block counter=1
    AESaux[5] = dndir?1:0;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
    os_copyMem(AESkey,key,16);
    os_aes(AES_CTR, payload, len);
}


static void aes_sessKeys (u2_t devnonce, xref2cu1_t artnonce, xref2u1_t nwkkey, xref2u1_t artkey) {
    os_clearMem(nwkkey, 16);
    nwkkey[0] = 0x01;
    os_copyMem(nwkkey+1, artnonce, LEN_ARTNONCE+LEN_NETID);
    os_wlsbf2(nwkkey+1+LEN_ARTNONCE+LEN_NETID, devnonce);
    os_copyMem(artkey, nwkkey, 16);
    artkey[0] = 0x02;

    os_getDevKey(AESkey);
    os_aes(AES_ENC, nwkkey, 16);
    os_getDevKey(AESkey);
    os_aes(AES_ENC, artkey, 16);
}

// END AES
// ================================================================================


// ================================================================================
// BEG LORA

static CONST_TABLE(u1_t, SENSITIVITY)[7][3] = {
    // ------------bw----------
    // 125kHz    250kHz    500kHz
    { 141-109,  141-109, 141-109 },  // FSK
    { 141-127,  141-124, 141-121 },  // SF7
    { 141-129,  141-126, 141-123 },  // SF8
    { 141-132,  141-129, 141-126 },  // SF9
    { 141-135,  141-132, 141-129 },  // SF10
    { 141-138,  141-135, 141-132 },  // SF11
    { 141-141,  141-138, 141-135 }   // SF12
};

int getSensitivity (rps_t rps) {
    return -141 + TABLE_GET_U1_TWODIM(SENSITIVITY, getSf(rps), getBw(rps));
}

ostime_t calcAirTime (rps_t rps, u1_t plen) {
    u1_t bw = getBw(rps);  // 0,1,2 = 125,250,500kHz
    u1_t sf = getSf(rps);  // 0=FSK, 1..6 = SF7..12
    if( sf == FSK ) {
        return (plen+/*preamble*/5+/*syncword*/3+/*len*/1+/*crc*/2) * /*bits/byte*/8
            * (s4_t)OSTICKS_PER_SEC / /*kbit/s*/50000;
    }
    u1_t sfx = 4*(sf+(7-SF7));
    u1_t q = sfx - (sf >= SF11 ? 8 : 0);
    int tmp = 8*plen - sfx + 28 + (getNocrc(rps)?0:16) - (getIh(rps)?20:0);
    if( tmp > 0 ) {
        tmp = (tmp + q - 1) / q;
        tmp *= getCr(rps)+5;
        tmp += 8;
    } else {
        tmp = 8;
    }
    tmp = (tmp<<2) + /*preamble*/49 /* 4 * (8 + 4.25) */;
    // bw = 125000 = 15625 * 2^3
    //      250000 = 15625 * 2^4
    //      500000 = 15625 * 2^5
    // sf = 7..12
    //
    // osticks =  tmp * OSTICKS_PER_SEC * 1<<sf / bw
    //
    // 3 => counter reduced divisor 125000/8 => 15625
    // 2 => counter 2 shift on tmp
    sfx = sf+(7-SF7) - (3+2) - bw;
    int div = 15625;
    if( sfx > 4 ) {
        // prevent 32bit signed int overflow in last step
        div >>= sfx-4;
        sfx = 4;
    }
    // Need 32bit arithmetic for this last step
    return (((ostime_t)tmp << sfx) * OSTICKS_PER_SEC + div/2) / div;
}

// END LORA
// ================================================================================


// Table below defines the size of one symbol as
//   symtime = 256us * 2^T(sf,bw)
// 256us is called one symunit.
//                 SF:
//      BW:      |__7___8___9__10__11__12
//      125kHz   |  2   3   4   5   6   7
//      250kHz   |  1   2   3   4   5   6
//      500kHz   |  0   1   2   3   4   5
//

static void setRxsyms (ostime_t rxsyms) {
    if (rxsyms >= (((ostime_t)1) << 10u)) {
        LMIC.rxsyms = (1u << 10u) - 1;
    } else if (rxsyms < 0) {
        LMIC.rxsyms = 0;
    } else {
        LMIC.rxsyms = rxsyms;
    }
}

#if !defined(DISABLE_BEACONS)
static ostime_t calcRxWindow (u1_t secs, dr_t dr) {
    ostime_t rxoff, err;
    if( secs==0 ) {
        // aka 128 secs (next becaon)
        rxoff = LMIC.drift;
        err = LMIC.lastDriftDiff;
    } else {
        // scheduled RX window within secs into current beacon period
        rxoff = (LMIC.drift * (ostime_t)secs) >> BCN_INTV_exp;
        err = (LMIC.lastDriftDiff * (ostime_t)secs) >> BCN_INTV_exp;
    }
    rxsyms_t rxsyms = LMICbandplan_MINRX_SYMS_LoRa_ClassB;
    err += (ostime_t)LMIC.maxDriftDiff * LMIC.missedBcns;
    setRxsyms(LMICbandplan_MINRX_SYMS_LoRa_ClassB + (err / dr2hsym(dr)));

    return (rxsyms-LMICbandplan_PAMBL_SYMS) * dr2hsym(dr) + rxoff;
}


// Setup beacon RX parameters assuming we have an error of ms (aka +/-(ms/2))
static void calcBcnRxWindowFromMillis (u1_t ms, bit_t ini) {
    if( ini ) {
        LMIC.drift = 0;
        LMIC.maxDriftDiff = 0;
        LMIC.missedBcns = 0;
        LMIC.bcninfo.flags |= BCN_NODRIFT|BCN_NODDIFF;
    }
    ostime_t hsym = dr2hsym(DR_BCN);
    LMIC.bcnRxsyms = LMICbandplan_MINRX_SYMS_LoRa_ClassB + ms2osticksCeil(ms) / hsym;
    LMIC.bcnRxtime = LMIC.bcninfo.txtime + BCN_INTV_osticks - (LMIC.bcnRxsyms-LMICbandplan_PAMBL_SYMS) * hsym;
}
#endif // !DISABLE_BEACONS


#if !defined(DISABLE_PING)
// Setup scheduled RX window (ping/multicast slot)
static void rxschedInit (xref2rxsched_t rxsched) {
    os_clearMem(AESkey,16);
    os_clearMem(LMIC.frame+8,8);
    os_wlsbf4(LMIC.frame, LMIC.bcninfo.time);
    os_wlsbf4(LMIC.frame+4, LMIC.devaddr);
    os_aes(AES_ENC,LMIC.frame,16);
    u1_t intvExp = rxsched->intvExp;
    ostime_t off = os_rlsbf2(LMIC.frame) & (0x0FFF >> (7 - intvExp)); // random offset (slot units)
    rxsched->rxbase = (LMIC.bcninfo.txtime +
                       BCN_RESERVE_osticks +
                       ms2osticks(BCN_SLOT_SPAN_ms * off)); // random offset osticks
    rxsched->slot   = 0;
    rxsched->rxtime = rxsched->rxbase - calcRxWindow(/*secs BCN_RESERVE*/2+(1<<intvExp),rxsched->dr);
    rxsched->rxsyms = LMIC.rxsyms;
}


static bit_t rxschedNext (xref2rxsched_t rxsched, ostime_t cando) {
  again:
    if( rxsched->rxtime - cando >= 0 )
        return 1;
    u1_t slot;
    if( (slot=rxsched->slot) >= 128 )
        return 0;
    u1_t intv = 1<<rxsched->intvExp;
    if( (rxsched->slot = (slot += (intv))) >= 128 )
        return 0;
    rxsched->rxtime = rxsched->rxbase
        + ((BCN_WINDOW_osticks * (ostime_t)slot) >> BCN_INTV_exp)
        - calcRxWindow(/*secs BCN_RESERVE*/2+slot+intv,rxsched->dr);
    rxsched->rxsyms = LMIC.rxsyms;
    goto again;
}
#endif // !DISABLE_PING)


ostime_t LMICcore_rndDelay (u1_t secSpan) {
    u2_t r = os_getRndU2();
    ostime_t delay = r;
    if( delay > OSTICKS_PER_SEC )
        delay = r % (u2_t)OSTICKS_PER_SEC;
    if( secSpan > 0 )
        delay += ((u1_t)r % secSpan) * OSTICKS_PER_SEC;
    return delay;
}

// delay reftime ticks, plus a random interval in [0..secSpan).
static void txDelay (ostime_t reftime, u1_t secSpan) {
    if (secSpan != 0)
        reftime += LMICcore_rndDelay(secSpan);
    if( LMIC.globalDutyRate == 0  ||  (reftime - LMIC.globalDutyAvail) > 0 ) {
        LMIC.globalDutyAvail = reftime;
        LMIC.opmode |= OP_RNDTX;
    }
}


void LMICcore_setDrJoin (u1_t reason, u1_t dr) {
    LMIC_EV_PARAMETER(reason);

    EV(drChange, INFO, (e_.reason    = reason,
                        e_.deveui    = MAIN::CDEV->getEui(),
                        e_.dr        = dr|DR_PAGE,
                        e_.txpow     = LMIC.adrTxPow,
                        e_.prevdr    = LMIC.datarate|DR_PAGE,
                        e_.prevtxpow = LMIC.adrTxPow));
    LMIC.datarate = dr;
    DO_DEVDB(LMIC.datarate,datarate);
}


static bit_t setDrTxpow (u1_t reason, u1_t dr, s1_t pow) {
    bit_t result = 0;

    LMIC_EV_PARAMETER(reason);

    EV(drChange, INFO, (e_.reason    = reason,
                        e_.deveui    = MAIN::CDEV->getEui(),
                        e_.dr        = dr|DR_PAGE,
                        e_.txpow     = pow,
                        e_.prevdr    = LMIC.datarate|DR_PAGE,
                        e_.prevtxpow = LMIC.adrTxPow));

    if( pow != KEEP_TXPOW && pow != LMIC.adrTxPow ) {
        LMIC.adrTxPow = pow;
        result = 1;
    }
    if( LMIC.datarate != dr ) {
        LMIC.datarate = dr;
        DO_DEVDB(LMIC.datarate,datarate);
        LMIC.opmode |= OP_NEXTCHNL;
        result = 1;
    }
    return result;
}


#if !defined(DISABLE_PING)
void LMIC_stopPingable (void) {
    LMIC.opmode &= ~(OP_PINGABLE|OP_PINGINI);
}


void LMIC_setPingable (u1_t intvExp) {
    // Change setting
    LMIC.ping.intvExp = (intvExp & 0x7);
    LMIC.opmode |= OP_PINGABLE;
    // App may call LMIC_enableTracking() explicitely before
    // Otherwise tracking is implicitly enabled here
    if( (LMIC.opmode & (OP_TRACK|OP_SCAN)) == 0  &&  LMIC.bcninfoTries == 0 )
        LMIC_enableTracking(0);
}

#endif // !DISABLE_PING

static void runEngineUpdate (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    engineUpdate();
}

static void reportEventAndUpdate(ev_t ev) {
    reportEventNoUpdate(ev);
    engineUpdate();
}

static void reportEventNoUpdate (ev_t ev) {
    uint32_t const evSet = UINT32_C(1) << ev;
    EV(devCond, INFO, (e_.reason = EV::devCond_t::LMIC_EV,
                       e_.eui    = MAIN::CDEV->getEui(),
                       e_.info   = ev));
#if LMIC_ENABLE_onEvent
    void (*pOnEvent)(ev_t) = onEvent;

    // rxstart is critical timing; legacy onEvent handlers
    // don't comprehend this; so don't report.
    if (pOnEvent != NULL && (evSet & (UINT32_C(1)<<EV_RXSTART)) == 0)
        pOnEvent(ev);
#endif // LMIC_ENABLE_onEvent

    // we want people who need tiny RAM footprints to be able
    // to use onEvent and overide the dynamic mechanism.
#if LMIC_ENABLE_user_events
    // create a mask to test against sets of events.

    // if a message was received, notify the user.
    if ((evSet & ((UINT32_C(1)<<EV_TXCOMPLETE) | (UINT32_C(1)<<EV_RXCOMPLETE))) != 0 &&
        LMIC.client.rxMessageCb != NULL &&
        (LMIC.dataLen  != 0 || LMIC.dataBeg != 0)) {
        uint8_t port;

        // assume no port.
        port = 0;

        // correct assumption if a port was provided.
        if (LMIC.txrxFlags & TXRX_PORT)
            port = LMIC.frame[LMIC.dataBeg - 1];

        // notify the user.
        LMIC.client.rxMessageCb(
                LMIC.client.rxMessageUserData,
                port,
                LMIC.frame + LMIC.dataBeg,
                LMIC.dataLen
                );
    }

    // tell the client about completed transmits -- the buffer
    // is now available again.  We use set notation again in case
    // we later discover another event completes messages
    if ((evSet & ((UINT32_C(1)<<EV_TXCOMPLETE) | (UINT32_C(1) <<EV_TXCANCELED))) != 0) {
        lmic_txmessage_cb_t * const pTxMessageCb = LMIC.client.txMessageCb;

        if (pTxMessageCb != NULL) {
            int fSuccess;
            // reset before notifying user. If we reset after
            // notifying, then if user does a recursive call
            // in their message processing
            // function, we would clobber the value
            LMIC.client.txMessageCb = NULL;

            // compute exit status
            if (ev == EV_TXCANCELED || (LMIC.txrxFlags & TXRX_LENERR) != 0) {
                // canceled, or killed because of length error: unsuccessful.
                fSuccess = 0;
            } else if (/* ev == EV_TXCOMPLETE  && */ LMIC.pendTxConf) {
                fSuccess = (LMIC.txrxFlags & TXRX_ACK) != 0;
            } else {
                // unconfirmed uplinks are successful if they were sent.
                fSuccess = 1;
            }

            // notify the user.
            pTxMessageCb(LMIC.client.txMessageUserData, fSuccess);
        }
    }

    // tell the client about events in general
    if (LMIC.client.eventCb != NULL)
        LMIC.client.eventCb(LMIC.client.eventUserData, ev);
#endif // LMIC_ENABLE_user_events
}

int LMIC_registerRxMessageCb(lmic_rxmessage_cb_t *pRxMessageCb, void *pUserData) {
#if LMIC_ENABLE_user_events
    LMIC.client.rxMessageCb = pRxMessageCb;
    LMIC.client.rxMessageUserData = pUserData;
    return 1;
#else // !LMIC_ENABLE_user_events
    return 0;
#endif // !LMIC_ENABLE_user_events
}

int LMIC_registerEventCb(lmic_event_cb_t *pEventCb, void *pUserData) {
#if LMIC_ENABLE_user_events
    LMIC.client.eventCb = pEventCb;
    LMIC.client.eventUserData = pUserData;
    return 1;
#else // ! LMIC_ENABLE_user_events
    return 0;
#endif // ! LMIC_ENABLE_user_events
}

static void runReset (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    // clear pending TX.
    LMIC_clrTxData();

    // Disable session
    LMIC_reset();

    // report event before the join event.
    reportEventNoUpdate(EV_RESET);

#if !defined(DISABLE_JOIN)
    LMIC_startJoining();
#else
    os_setCallback(&LMIC.osjob, FUNC_ADDR(runEngineUpdate));
#endif // !DISABLE_JOIN
}

static void resetJoinParams(void) {
    LMIC.rx1DrOffset = 0;
    LMIC.dn2Dr       = DR_DNW2;
    LMIC.dn2Freq     = FREQ_DNW2;
#if LMIC_ENABLE_TxParamSetupReq
    LMIC.txParam     = 0xFF;
#endif
}

static void stateJustJoined (void) {
    LMIC.seqnoDn     = LMIC.seqnoUp = 0;
    LMIC.rejoinCnt   = 0;
    LMIC.dnConf      = LMIC.lastDnConf  = LMIC.adrChanged = 0;
    LMIC.upRepeatCount = LMIC.upRepeat = 0;
#if !defined(DISABLE_MCMD_RXParamSetupReq)
    LMIC.dn2Ans      = 0;
#endif
#if !defined(DISABLE_MCMD_RXTimingSetupReq)
    LMIC.macRxTimingSetupAns = 0;
#endif
#if !defined(DISABLE_MCMD_DlChannelReq) && CFG_LMIC_EU_like
    LMIC.macDlChannelAns = 0;
#endif
    LMIC.moreData    = 0;
    LMIC.upRepeat    = 0;
    resetJoinParams();
#if !defined(DISABLE_BEACONS)
    LMIC.bcnChnl     = CHNL_BCN;
#endif
#if !defined(DISABLE_PING)
    LMIC.ping.freq   = FREQ_PING;
    LMIC.ping.dr     = DR_PING;
#endif
}


// ================================================================================
// Decoding frames


#if !defined(DISABLE_BEACONS)
// Decode beacon  - do not overwrite bcninfo unless we have a match!
static lmic_beacon_error_t decodeBeacon (void) {
    if (LMIC.dataLen != LEN_BCN) { // implicit header RX guarantees this
        return LMIC_BEACON_ERROR_INVALID;
    }
    xref2u1_t d = LMIC.frame;
    if(! LMICbandplan_isValidBeacon1(d))
        return LMIC_BEACON_ERROR_INVALID;   // first (common) part fails CRC check
    // First set of fields is ok
    u4_t bcnnetid = os_rlsbf4(&d[OFF_BCN_NETID]) & 0xFFFFFF;
    if( bcnnetid != LMIC.netid )
        return LMIC_BEACON_ERROR_WRONG_NETWORK;  // not the beacon we're looking for

    LMIC.bcninfo.flags &= ~(BCN_PARTIAL|BCN_FULL);
    // Match - update bcninfo structure
    LMIC.bcninfo.snr    = LMIC.snr;
    LMIC.bcninfo.rssi   = LMIC.rssi;
    LMIC.bcninfo.txtime = LMIC.rxtime - AIRTIME_BCN_osticks;
    LMIC.bcninfo.time   = os_rlsbf4(&d[OFF_BCN_TIME]);
    LMIC.bcninfo.flags |= BCN_PARTIAL;

    // Check 2nd set
    if( os_rlsbf2(&d[OFF_BCN_CRC2]) != os_crc16(d,OFF_BCN_CRC2) )
        return LMIC_BEACON_ERROR_SUCCESS_PARTIAL;
    // Second set of fields is ok
    LMIC.bcninfo.lat    = (s4_t)os_rlsbf4(&d[OFF_BCN_LAT-1]) >> 8; // read as signed 24-bit
    LMIC.bcninfo.lon    = (s4_t)os_rlsbf4(&d[OFF_BCN_LON-1]) >> 8; // ditto
    LMIC.bcninfo.info   = d[OFF_BCN_INFO];
    LMIC.bcninfo.flags |= BCN_FULL;
    return LMIC_BEACON_ERROR_SUCCESS_FULL;
}
#endif // !DISABLE_BEACONS

// put a mac response to the current output buffer. Limit according to kind of
// mac data (piggyback vs port 0)
static bit_t put_mac_uplink_byte(uint8_t b) {
    if (LMIC.pendMacPiggyback) {
        // put in pendMacData
        if (LMIC.pendMacLen < sizeof(LMIC.pendMacData)) {
            LMIC.pendMacData[LMIC.pendMacLen++] = b;
            return 1;
        } else {
            return 0;
        }
    } else {
        // put in pendTxData
        if (LMIC.pendMacLen < sizeof(LMIC.pendTxData)) {
            LMIC.pendTxData[LMIC.pendMacLen++] = b;
            return 1;
        } else {
            return 0;
        }
    }
}

static bit_t put_mac_uplink_byte2(uint8_t b1, uint8_t b2) {
    u1_t outindex = LMIC.pendMacLen;

    if (put_mac_uplink_byte(b1) && put_mac_uplink_byte(b2)) {
        return 1;
    } else {
        LMIC.pendMacLen = outindex;
        return 0;
    }
}

static bit_t put_mac_uplink_byte3(u1_t b1, u1_t b2, u1_t b3) {
    u1_t outindex = LMIC.pendMacLen;

    if (put_mac_uplink_byte(b1) && put_mac_uplink_byte(b2) && put_mac_uplink_byte(b3)) {
        return 1;
    } else {
        LMIC.pendMacLen = outindex;
        return 0;
    }
}

static CONST_TABLE(u1_t, macCmdSize)[] = {
    /* 2: LinkCheckAns */ 3,
    /* 3: LinkADRReq */ 5,
    /* 4: DutyCycleReq */ 2,
    /* 5: RXParamSetupReq */ 5,
    /* 6: DevStatusReq */ 1,
    /* 7: NewChannelReq */ 6,
    /* 8: RXTimingSetupReq */ 2,
    /* 9: TxParamSetupReq */ 2,
    /* 0x0A: DlChannelReq */ 5,
    /* B, C: RFU */ 0, 0,
    /* 0x0D: DeviceTimeAns */ 6,
    /* 0x0E, 0x0F */ 0, 0,
    /* 0x10: PingSlotInfoAns */ 1,
    /* 0x11: PingSlotChannelReq */ 5,
    /* 0x12: BeaconTimingAns */ 4,
    /* 0x13: BeaconFreqReq */ 4
};

static u1_t getMacCmdSize(u1_t macCmd) {
    if (macCmd >= 2) {
        const unsigned macCmdMinus2 = macCmd - 2u;
        if (macCmdMinus2 < LENOF_TABLE(macCmdSize)) {
            // macCmd in table, fetch it's size.
            return TABLE_GET_U1(macCmdSize, macCmdMinus2);
        }
    }
    // macCmd too small or too large: return zero. Zero is
    // never a legal command size, so it signals an error
    // to the caller.
    return 0;
}

static bit_t
applyAdrRequests(
    const uint8_t *opts,
    int olen,
    u1_t adrAns
) {
    lmic_saved_adr_state_t initialState;
    int const kAdrReqSize = 5;
    int oidx;
    u1_t p1 = 0;
    u1_t p4 = 0;
    bit_t response_fit = 1;
    bit_t map_ok = 1;

    LMICbandplan_saveAdrState(&initialState);

    // compute the changes
    if (adrAns == (MCMD_LinkADRAns_PowerACK | MCMD_LinkADRAns_DataRateACK | MCMD_LinkADRAns_ChannelACK)) {
        for (oidx = 0; oidx < olen; oidx += kAdrReqSize) {
            // can we advance?
            if (olen - oidx < kAdrReqSize) {
                // ignore the malformed one at the end
                break;
            }
            u2_t chmap  = os_rlsbf2(&opts[oidx+2]);// list of enabled channels

            p1     = opts[oidx+1];                  // txpow + DR, in case last
            p4     = opts[oidx+4];                  // ChMaskCtl, NbTrans
            u1_t chpage = p4 & MCMD_LinkADRReq_Redundancy_ChMaskCntl_MASK;     // channel page

            map_ok = LMICbandplan_mapChannels(chpage, chmap);
            LMICOS_logEventUint32("applyAdrRequests: mapChannels", ((u4_t)chpage << 16)|(chmap << 0));
        }
    }

    if (! map_ok) {
        adrAns &= ~MCMD_LinkADRAns_ChannelACK;
    }

    // p1 now has txpow + DR. DR must be feasible.
    dr_t dr = (dr_t)(p1>>MCMD_LinkADRReq_DR_SHIFT);

    if (adrAns == (MCMD_LinkADRAns_PowerACK | MCMD_LinkADRAns_DataRateACK | MCMD_LinkADRAns_ChannelACK) && ! LMICbandplan_isDataRateFeasible(dr)) {
        adrAns &= ~MCMD_LinkADRAns_DataRateACK;
        LMICOS_logEventUint32("applyAdrRequests: final DR not feasible", dr);
    }

    if (adrAns != (MCMD_LinkADRAns_PowerACK | MCMD_LinkADRAns_DataRateACK | MCMD_LinkADRAns_ChannelACK)) {
        LMICbandplan_restoreAdrState(&initialState);
    }

    // now put all the options
    for (oidx = 0; oidx < olen && response_fit; oidx += kAdrReqSize) {
        // can we advance?
        if (olen - oidx < kAdrReqSize) {
            // ignore the malformed one at the end
            break;
        }
        response_fit = put_mac_uplink_byte2(MCMD_LinkADRAns, adrAns);
    }

    // all done scanning options
    bit_t changes = LMICbandplan_compareAdrState(&initialState);

    // handle the final options
    if (adrAns == (MCMD_LinkADRAns_PowerACK | MCMD_LinkADRAns_DataRateACK | MCMD_LinkADRAns_ChannelACK)) {
        // handle uplink repeat count
        u1_t uprpt  = p4 & MCMD_LinkADRReq_Redundancy_NbTrans_MASK;     // up repeat count
        if (LMIC.upRepeat != uprpt) {
            LMIC.upRepeat = uprpt;
            changes = 1;
        }

        LMICOS_logEventUint32("applyAdrRequests: setDrTxPow", ((u4_t)adrAns << 16)|(dr << 8)|(p1 << 0));

        // handle power changes here, too.
        changes |= setDrTxpow(DRCHG_NWKCMD, dr, pow2dBm(p1));
    }

    // Certification doesn't like this, but it makes the device happier with TTN.
    // LMIC.adrChanged = changes;  // move the ADR FSM up to "time to request"

    return response_fit;
}

static int
scan_mac_cmds_link_adr(
    const uint8_t *opts,
    int olen,
    bit_t *presponse_fit
    )
    {
    LMICOS_logEventUint32("scan_mac_cmds_link_adr", olen);

    if (olen == 0)
        return 0;

    int oidx = 0;
    int const kAdrReqSize = 5;
    int lastOidx;
    u1_t adrAns = MCMD_LinkADRAns_PowerACK | MCMD_LinkADRAns_DataRateACK | MCMD_LinkADRAns_ChannelACK;

    // process the contiguous slots
    for (;;) {
        lastOidx = oidx;

        // can we advance?
        if (olen - oidx < kAdrReqSize) {
            // ignore the malformed one at the end; but fail it.
            adrAns = 0;
            break;
        }
        u1_t p1     = opts[oidx+1];            // txpow + DR
        u2_t chmap  = os_rlsbf2(&opts[oidx+2]);// list of enabled channels
        u1_t chpage = opts[oidx+4] & MCMD_LinkADRReq_Redundancy_ChMaskCntl_MASK;     // channel page
        // u1_t uprpt  = opts[oidx+4] & MCMD_LinkADRReq_Redundancy_NbTrans_MASK;     // up repeat count
        dr_t dr = (dr_t)(p1>>MCMD_LinkADRReq_DR_SHIFT);

        if( !LMICbandplan_canMapChannels(chpage, chmap) ) {
            adrAns &= ~MCMD_LinkADRAns_ChannelACK;
            LMICOS_logEventUint32("scan_mac_cmds_link_adr: failed canMapChannels", ((u4_t)chpage << 16)|((u4_t)chmap << 0));
        }

        if( !validDR(dr) ) {
            adrAns &= ~MCMD_LinkADRAns_DataRateACK;
        }
        if (pow2dBm(p1) == -128) {
            adrAns &= ~MCMD_LinkADRAns_PowerACK;
        }

        oidx += kAdrReqSize;
        if (opts[oidx] != MCMD_LinkADRReq)
            break;
    }

    // go back and apply the ADR changes, if any -- use the effective length,
    // and process.
    *presponse_fit = applyAdrRequests(opts, lastOidx + kAdrReqSize, adrAns);

    return lastOidx;
    }

// scan mac commands starting at opts[] for olen, return count of bytes consumed.
// build response in pendMacData[], but limit length as needed; simply chop at last
// response that fits.
static int
scan_mac_cmds(
    const uint8_t *opts,
    int olen,
    int port
    ) {
    int oidx = 0;
    uint8_t cmd;

    LMIC.pendMacLen = 0;
    if (port == 0) {
        // port zero: mac data is in the normal payload, and there can't be
        // piggyback mac data.
        LMIC.pendMacPiggyback = 0;
    } else {
        // port is either -1 (no port) or non-zero (piggyback): treat as piggyback.
        LMIC.pendMacPiggyback = 1;
    }

    while( oidx < olen ) {
        bit_t response_fit;

        response_fit = 1;
        cmd = opts[oidx];

        /* compute length, and exit for illegal commands */
        // cmdlen == 0 for error, or > 0 length of command.
        int const cmdlen = getMacCmdSize(cmd);
        if (cmdlen <= 0 || cmdlen > olen - oidx) {
            // "the first unknown command terminates processing"
            olen = oidx;
            break;
        }

        switch( cmd ) {
        case MCMD_LinkCheckAns: {
            // TODO(tmm@mcci.com) capture these, reliably..
            //int gwmargin = opts[oidx+1];
            //int ngws = opts[oidx+2];
            break;
        }
        // from 1.0.3 spec section 5.2:
        // For the purpose of configuring the end-device channel mask, the end-device will
        // process all contiguous LinkAdrReq messages, in the order present in the downlink message,
        // as a single atomic block command. The end-device will accept or reject all Channel Mask
        // controls in the contiguous block, and provide consistent Channel Mask ACK status
        // indications for each command in the contiguous block in each LinkAdrAns message,
        // reflecting the acceptance or rejection of this atomic channel mask setting.
        //
        // So we need to process all the contigious commands
        case MCMD_LinkADRReq: {
            // skip over all but the last command.
            oidx += scan_mac_cmds_link_adr(opts + oidx, olen - oidx, &response_fit);
            break;
        }

        case MCMD_DevStatusReq: {
            // LMIC.snr is SNR times 4, convert to real SNR; rounding towards zero.
            const int snr = (LMIC.snr + 2) / 4;
            // per [1.02] 5.5. the margin is the SNR.
            LMIC.devAnsMargin = (u1_t)(0b00111111 & (snr <= -32 ? -32 : snr >= 31 ? 31 : snr));

            response_fit = put_mac_uplink_byte3(MCMD_DevStatusAns, os_getBattLevel(), LMIC.devAnsMargin);
            break;
        }

#if !defined(DISABLE_MCMD_RXParamSetupReq)
        case MCMD_RXParamSetupReq: {
            dr_t dr = (dr_t)(opts[oidx+1] & 0x0F);
            u1_t rx1DrOffset = (u1_t)((opts[oidx+1] & 0x70) >> 4);
            u4_t freq = LMICbandplan_convFreq(&opts[oidx+2]);
            LMIC.dn2Ans = 0xC0;   // answer pending, but send this one in order.
            if( validDR(dr) )
                LMIC.dn2Ans |= MCMD_RXParamSetupAns_RX2DataRateACK;
            if( freq != 0 )
                LMIC.dn2Ans |= MCMD_RXParamSetupAns_ChannelACK;
            if (rx1DrOffset <= 3)
                LMIC.dn2Ans |= MCMD_RXParamSetupAns_RX1DrOffsetAck;

            if( LMIC.dn2Ans == (0xC0|MCMD_RXParamSetupAns_RX2DataRateACK|MCMD_RXParamSetupAns_ChannelACK| MCMD_RXParamSetupAns_RX1DrOffsetAck) ) {
                LMIC.dn2Dr = dr;
                LMIC.dn2Freq = freq;
                LMIC.rx1DrOffset = rx1DrOffset;
                DO_DEVDB(LMIC.dn2Dr,dn2Dr);
                DO_DEVDB(LMIC.dn2Freq,dn2Freq);
            }

            /* put the first copy of the message */
            response_fit = put_mac_uplink_byte2(MCMD_RXParamSetupAns, LMIC.dn2Ans & ~MCMD_RXParamSetupAns_RFU);
            break;
        }
#endif // !DISABLE_MCMD_RXParamSetupReq

#if !defined(DISABLE_MCMD_RXTimingSetupReq)
        case MCMD_RXTimingSetupReq: {
            u1_t delay = opts[oidx+1] & MCMD_RXTimingSetupReq_Delay;
            if (delay == 0)
                delay = 1;

            LMIC.rxDelay = delay;
            LMIC.macRxTimingSetupAns = 2;
            response_fit = put_mac_uplink_byte(MCMD_RXTimingSetupAns);
            break;
        }
#endif // !DISABLE_MCMD_RXTimingSetupReq

#if !defined(DISABLE_MCMD_DutyCycleReq)
        case MCMD_DutyCycleReq: {
            u1_t cap = opts[oidx+1];
            LMIC.globalDutyRate  = cap & 0xF;
            LMIC.globalDutyAvail = os_getTime();
            DO_DEVDB(cap,dutyCap);

            response_fit = put_mac_uplink_byte(MCMD_DutyCycleAns);
            break;
        }
#endif // !DISABLE_MCMD_DutyCycleReq

#if !defined(DISABLE_MCMD_NewChannelReq) && CFG_LMIC_EU_like
        case MCMD_NewChannelReq: {
            u1_t chidx = opts[oidx+1];  // channel
            u4_t raw_f_not_zero = opts[oidx+2] | opts[oidx+3] | opts[oidx+4];
            u4_t freq  = LMICbandplan_convFreq(&opts[oidx+2]); // freq
            u1_t drs   = opts[oidx+5];  // datarate span
            u1_t ans   = MCMD_NewChannelAns_DataRateACK|MCMD_NewChannelAns_ChannelACK;

            if (freq == 0 && raw_f_not_zero) {
                ans &= ~MCMD_NewChannelAns_ChannelACK;
            }
            u1_t MaxDR = drs >> 4;
            u1_t MinDR = drs & 0xF;
            if (MaxDR < MinDR || !validDR(MaxDR) || !validDR(MinDR)) {
                ans &= ~MCMD_NewChannelAns_DataRateACK;
            }

            if( ans == (MCMD_NewChannelAns_DataRateACK|MCMD_NewChannelAns_ChannelACK)) {
                if ( ! LMIC_setupChannel(chidx, freq, DR_RANGE_MAP(MinDR, MaxDR), -1) ) {
                    LMICOS_logEventUint32("NewChannelReq: setupChannel failed", ((u4_t)MaxDR << 24u) | ((u4_t)MinDR << 16u) | (raw_f_not_zero << 8) | (chidx << 0));
                    ans &= ~MCMD_NewChannelAns_ChannelACK;
                }
            }

            response_fit = put_mac_uplink_byte2(MCMD_NewChannelAns, ans);
            break;
        }
#endif // !DISABLE_MCMD_NewChannelReq

#if !defined(DISABLE_MCMD_DlChannelReq) && CFG_LMIC_EU_like
        case MCMD_DlChannelReq: {
            u1_t chidx = opts[oidx+1];  // channel
            u4_t freq  = LMICbandplan_convFreq(&opts[oidx+2]); // freq
            u1_t ans   = MCMD_DlChannelAns_FreqACK|MCMD_DlChannelAns_ChannelACK;

            if (freq == 0) {
                ans &= ~MCMD_DlChannelAns_ChannelACK;
            }
            if (chidx > MAX_CHANNELS) {
                // this is not defined by the 1.0.3 spec
                ans = 0;
            } else if ((LMIC.channelMap & (1 << chidx)) == 0) {
                // the channel is not enabled for downlink.
                ans &= ~MCMD_DlChannelAns_FreqACK;
            }

            if( ans == (MCMD_DlChannelAns_FreqACK|MCMD_DlChannelAns_ChannelACK)) {
                LMIC.channelDlFreq[chidx] = freq;
            }

            response_fit = put_mac_uplink_byte2(MCMD_DlChannelAns, ans);
            // set sticky answer.
            LMIC.macDlChannelAns = ans | 0xC0;
            break;
        }
#endif // !DISABLE_MCMD_DlChannelReq

#if !defined(DISABLE_MCMD_PingSlotChannelReq) && !defined(DISABLE_PING)
        case MCMD_PingSlotChannelReq: {
            u4_t raw_f_not_zero = opts[oidx+1] | opts[oidx+2] | opts[oidx+3];
            u4_t freq = LMICbandplan_convFreq(&opts[oidx+1]);
            u1_t dr = opts[oidx+4] & 0xF;
            u1_t ans = MCMD_PingSlotFreqAns_DataRateACK|MCMD_PingSlotFreqAns_ChannelACK;
            if (! raw_f_not_zero) {
                freq = FREQ_PING;
            } else if (freq == 0) {
                ans &= ~MCMD_PingSlotFreqAns_ChannelACK;
            }
            if (! validDR(dr))
                ans &= ~MCMD_PingSlotFreqAns_DataRateACK;

            if (ans == (MCMD_PingSlotFreqAns_DataRateACK|MCMD_PingSlotFreqAns_ChannelACK)) {
                LMIC.ping.freq = freq;
                LMIC.ping.dr = dr;
                DO_DEVDB(LMIC.ping.intvExp, pingIntvExp);
                DO_DEVDB(LMIC.ping.freq, pingFreq);
                DO_DEVDB(LMIC.ping.dr, pingDr);
            }
            response_fit = put_mac_uplink_byte2(MCMD_PingSlotChannelAns, ans);
            break;
        }
#endif // !DISABLE_MCMD_PingSlotChannelReq && !DISABLE_PING

#if defined(ENABLE_MCMD_BeaconTimingAns) && !defined(DISABLE_BEACONS)
        case MCMD_BeaconTimingAns: {
            // Ignore if tracking already enabled or bcninfoTries == 0
            if( (LMIC.opmode & OP_TRACK) == 0 && LMIC.bcninfoTries != 0) {
                LMIC.bcnChnl = opts[oidx+3];
                // Enable tracking - bcninfoTries
                LMIC.opmode |= OP_TRACK;
                // LMIC.bcninfoTries is cleared later in txComplete handling - triggers EV_BEACON_FOUND
                // Setup RX parameters
                LMIC.bcninfo.txtime = (LMIC.rxtime
                                       + ms2osticks(os_rlsbf2(&opts[oidx+1]) * MCMD_BeaconTimingAns_TUNIT)
                                       + ms2osticksCeil(MCMD_BeaconTimingAns_TUNIT/2)
                                       - BCN_INTV_osticks);
                LMIC.bcninfo.flags = 0;  // txtime above cannot be used as reference (BCN_PARTIAL|BCN_FULL cleared)
                calcBcnRxWindowFromMillis(MCMD_BeaconTimingAns_TUNIT,1);  // error of +/-N ms

                EV(lostFrame, INFO, (e_.reason  = EV::lostFrame_t::MCMD_BeaconTimingAns,
                                     e_.eui     = MAIN::CDEV->getEui(),
                                     e_.lostmic = Base::lsbf4(&d[pend]),
                                     e_.info    = (LMIC.missedBcns |
                                                   (osticks2us(LMIC.bcninfo.txtime + BCN_INTV_osticks
                                                               - LMIC.bcnRxtime) << 8)),
                                     e_.time    = MAIN::CDEV->ostime2ustime(LMIC.bcninfo.txtime + BCN_INTV_osticks)));
            }
            break;
        } /* end case */
#endif // !ENABLE_MCMD_BeaconTimingAns && !DISABLE_BEACONS

#if LMIC_ENABLE_TxParamSetupReq
        case MCMD_TxParamSetupReq: {
            uint8_t txParam;
            txParam = opts[oidx+1];

            // we don't allow unrecognized bits to get to txParam.
            txParam &= (MCMD_TxParam_RxDWELL_MASK|
                        MCMD_TxParam_TxDWELL_MASK|
                        MCMD_TxParam_MaxEIRP_MASK);
            LMIC.txParam = txParam;
            response_fit = put_mac_uplink_byte(MCMD_TxParamSetupAns);
            break;
        } /* end case */
#endif // LMIC_ENABLE_TxParamSetupReq

#if LMIC_ENABLE_DeviceTimeReq
        case MCMD_DeviceTimeAns: {
            // don't process a spurious downlink.
            if ( LMIC.txDeviceTimeReqState == lmic_RequestTimeState_rx ) {
                // remember that it's time to notify the client.
                LMIC.txDeviceTimeReqState = lmic_RequestTimeState_success;

                // the network time is linked to the time of the last TX.
                LMIC.localDeviceTime = LMIC.txend;

                // save the network time.
                // The first 4 bytes contain the seconds since the GPS epoch
                // (i.e January the 6th 1980 at 00:00:00 UTC).
                // Note: as per the LoRaWAN specs, the octet order for all
                //       multi-octet fields is little endian
                // Note: the casts are necessary, because opts is an array of
                //       single byte values, and they might overflow when shifted
                LMIC.netDeviceTime = ( (lmic_gpstime_t) opts[oidx + 1]       ) |
                                     (((lmic_gpstime_t) opts[oidx + 2]) <<  8) |
                                     (((lmic_gpstime_t) opts[oidx + 3]) << 16) |
                                     (((lmic_gpstime_t) opts[oidx + 4]) << 24);

                // The 5th byte contains the fractional seconds in 2^-8 second steps
                LMIC.netDeviceTimeFrac = opts[oidx + 5];
#if LMIC_DEBUG_LEVEL > 0
                LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": MAC command DeviceTimeAns received: seconds_since_gps_epoch=%"PRIu32", fractional_seconds=%d\n", os_getTime(), LMIC.netDeviceTime, LMIC.netDeviceTimeFrac);
#endif
            }
            break;
        } /* end case */
#endif // LMIC_ENABLE_DeviceTimeReq

        default: {
            // force olen to current oidx so we'll exit the while()
            olen = oidx;
            break;
        } /* end case */
        } /* end switch */

        /* if we're out of spce for responses, skip to end. */
        if (! response_fit) {
            olen = oidx;
        } else {
            oidx += cmdlen;
        }
    } /* end while */

    return oidx;
}

// change the ADR ack request count, unless adr ack is diabled.
static void setAdrAckCount (s2_t count) {
    if (LMIC.adrAckReq != LINK_CHECK_OFF) {
        LMIC.adrAckReq = count;
    }
}

static bit_t decodeFrame (void) {
    xref2u1_t d = LMIC.frame;
    u1_t hdr    = d[0];
    u1_t ftype  = hdr & HDR_FTYPE;
    int  dlen   = LMIC.dataLen;
#if LMIC_DEBUG_LEVEL > 0
    const char *window = (LMIC.txrxFlags & TXRX_DNW1) ? "RX1" : ((LMIC.txrxFlags & TXRX_DNW2) ? "RX2" : "Other");
#endif
    if (dlen > 0)
        LMICOS_logEventUint32("decodeFrame", (dlen << 8) | (hdr << 0));

    if( dlen < OFF_DAT_OPTS+4 ||
        (hdr & HDR_MAJOR) != HDR_MAJOR_V1 ||
        (ftype != HDR_FTYPE_DADN  &&  ftype != HDR_FTYPE_DCDN) ) {
        // Basic sanity checks failed
        EV(specCond, WARN, (e_.reason = EV::specCond_t::UNEXPECTED_FRAME,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = dlen < 4 ? 0 : os_rlsbf4(&d[dlen-4]),
                            e_.info2  = hdr + (dlen<<8)));
      norx:
#if LMIC_DEBUG_LEVEL > 0
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": Invalid downlink, window=%s\n", os_getTime(), window);
#endif
        LMIC.dataLen = 0;
        return 0;
    }
    // Validate exact frame length
    // Note: device address was already read+evaluated in order to arrive here.
    int  fct   = d[OFF_DAT_FCT];
    u4_t addr  = os_rlsbf4(&d[OFF_DAT_ADDR]);
    u4_t seqno = os_rlsbf2(&d[OFF_DAT_SEQNO]);
    int  olen  = fct & FCT_OPTLEN;
    int  ackup = (fct & FCT_ACK) != 0 ? 1 : 0;   // ACK last up frame
    int  poff  = OFF_DAT_OPTS+olen;
    int  pend  = dlen-4;  // MIC

    if( addr != LMIC.devaddr ) {
        LMICOS_logEventUint32("decodeFrame: wrong address", addr);

        EV(specCond, WARN, (e_.reason = EV::specCond_t::ALIEN_ADDRESS,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = addr,
                            e_.info2  = LMIC.devaddr));
        goto norx;
    }
    if( poff > pend ) {
        LMICOS_logEventUint32("decodeFrame: corrupted frame", ((u4_t)dlen << 16) | (fct << 8) | (poff - pend));
        EV(specCond, ERR, (e_.reason = EV::specCond_t::CORRUPTED_FRAME,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = 0x1000000 + (poff-pend) + (fct<<8) + (dlen<<16)));
        goto norx;
    }

    int port = -1;
    int replayConf = 0;

    if( pend > poff )
        port = d[poff++];

    // compute the 32-bit sequence number based on the 16-bit sequence number received
    // and the internal 32-bit number.  Because the 32-bit number is used in the MIC
    // calculation, this must be right.  (And if you're curious why a 32-bit seqno matters,
    // it's this calculation, plus its use in the MIC calculation.)
    //
    // we have to be careful to get the right value for replay of last message received.
    u2_t seqnoDiff = (u2_t)(seqno - LMIC.seqnoDn);
    if (seqnoDiff == 0xFFFFu) {
        seqno = LMIC.seqnoDn - 1;
    } else {
        seqno = LMIC.seqnoDn + seqnoDiff;
    }

    if( !aes_verifyMic(LMIC.nwkKey, LMIC.devaddr, seqno, /*dn*/1, d, pend) ) {
        LMICOS_logEventUint32("decodeFrame: bad MIC", os_rlsbf4(&d[pend]));
        EV(spe3Cond, ERR, (e_.reason = EV::spe3Cond_t::CORRUPTED_MIC,
                           e_.eui1   = MAIN::CDEV->getEui(),
                           e_.info1  = Base::lsbf4(&d[pend]),
                           e_.info2  = seqno,
                           e_.info3  = LMIC.devaddr));
        goto norx;
    }
    if( seqno < LMIC.seqnoDn ) {
        if( (s4_t)seqno > (s4_t)LMIC.seqnoDn ) {
            EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_ROLL_OVER,
                                e_.eui    = MAIN::CDEV->getEui(),
                                e_.info   = LMIC.seqnoDn,
                                e_.info2  = seqno));
            LMICOS_logEventUint32("decodeFrame: rollover discarded", ((u4_t)seqno << 16) | (LMIC.lastDnConf << 8) | (ftype << 0));
            goto norx;
        }
        if( seqno != LMIC.seqnoDn-1 || !LMIC.lastDnConf || ftype != HDR_FTYPE_DCDN ) {
            EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_OBSOLETE,
                                e_.eui    = MAIN::CDEV->getEui(),
                                e_.info   = LMIC.seqnoDn,
                                e_.info2  = seqno));
            LMICOS_logEventUint32("decodeFrame: Retransmit confimed discarded", ((u4_t)seqno << 16) | (LMIC.lastDnConf << 8) | (ftype << 0));
            goto norx;
        }
        // Replay of previous sequence number allowed only if
        // previous frame and repeated both requested confirmation
        // but set a flag, so we don't actually process the message.
        LMICOS_logEventUint32("decodeFrame: Retransmit confimed accepted", ((u4_t)seqno << 16) | (LMIC.lastDnConf << 8) | (ftype << 0));
        replayConf = 1;
        LMIC.dnConf = FCT_ACK;
    }
    else {
        if( seqnoDiff > LMICbandplan_MAX_FCNT_GAP) {
            LMICOS_logEventUint32("decodeFrame: gap too big", ((u4_t)seqnoDiff << 16) | (seqno & 0xFFFFu));
            goto norx;
        }
        if( seqno > LMIC.seqnoDn ) {
            EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_SKIP,
                                e_.eui    = MAIN::CDEV->getEui(),
                                e_.info   = LMIC.seqnoDn,
                                e_.info2  = seqno));
        }
        LMIC.seqnoDn = seqno+1;  // next number to be expected
        DO_DEVDB(LMIC.seqnoDn,seqnoDn);
        // DN frame requested confirmation - provide ACK once with next UP frame
        LMIC.dnConf = LMIC.lastDnConf = (ftype == HDR_FTYPE_DCDN ? FCT_ACK : 0);
        if (LMIC.dnConf)
            LMICOS_logEventUint32("decodeFrame: Confirmed downlink", ((u4_t)seqno << 16) | (LMIC.lastDnConf << 8) | (ftype << 0));
    }

    if (port == 0 && olen != 0 && pend > poff) {
        // we have a port-zero message, and piggyback mac data.
        // discard, section 4.3.1.6 line 544-546
#if LMIC_DEBUG_LEVEL > 0
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": port==0 && FOptsLen=%#x: discard\n", os_getTime(), olen);
#endif
        goto norx;
    }

    if( LMIC.dnConf || (fct & FCT_MORE) )
        LMIC.opmode |= OP_POLL;

    // We heard from network
    LMIC.adrChanged = LMIC.rejoinCnt = 0;
    setAdrAckCount(LINK_CHECK_INIT);
#if !defined(DISABLE_MCMD_RXParamSetupReq)
    // We heard from network "on a Class A downlink"
    LMIC.dn2Ans = 0;
#endif // !defined(DISABLE_MCMD_RXParamSetupReq)
#if !defined(DISABLE_MCMD_RXTimingSetupReq)
    // We heard from network "on a Class A downlink"
    LMIC.macRxTimingSetupAns = 0;
#endif // !defined(DISABLE_MCMD_RXParamSetupReq)
#if !defined(DISABLE_MCMD_DlChannelReq) && CFG_LMIC_EU_like
    LMIC.macDlChannelAns = 0;
#endif

    int m = LMIC.rssi - RSSI_OFF - getSensitivity(LMIC.rps);
    // for legacy reasons, LMIC.margin is set to the unsigned sensitivity. It can never be negative.
    // it's only computed for legacy clients
    LMIC.margin = m < 0 ? 0 : m > 254 ? 254 : m;

    // even if it's a replay confirmed, we process the mac options.
    xref2u1_t opts = &d[OFF_DAT_OPTS];
    int oidx = scan_mac_cmds(opts, olen, port);
    if( oidx != olen ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::CORRUPTED_FRAME,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = 0x1000000 + (oidx) + (olen<<8)));
        oidx = olen;
    }

    if( !replayConf ) {
        // Handle payload only if not a replay
        // Decrypt payload - if any
        if( port >= 0  &&  pend-poff > 0 ) {
            aes_cipher(port <= 0 ? LMIC.nwkKey : LMIC.artKey, LMIC.devaddr, seqno, /*dn*/1, d+poff, pend-poff);
            if (port == 0) {
                // this is a mac command. scan the options.
#if LMIC_DEBUG_LEVEL > 0
                LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": process mac commands for port 0 (olen=%#x)\n", os_getTime(), pend-poff);
#endif
                int optendindex = scan_mac_cmds(d+poff, pend-poff, port);
                if (optendindex != pend-poff) {
#if LMIC_DEBUG_LEVEL > 0
                    LMIC_DEBUG_PRINTF(
                        "%"LMIC_PRId_ostime_t": error processing mac commands for port 0 "
                        "(len=%#x, optendindex=%#x)\n",
                        os_getTime(), pend-poff, optendindex
                        );
#endif
                }
                // wait to transmit until txcomplete: above.
            }
        } // end decrypt payload
        EV(dfinfo, DEBUG, (e_.deveui  = MAIN::CDEV->getEui(),
                           e_.devaddr = LMIC.devaddr,
                           e_.seqno   = seqno,
                           e_.flags   = (port < 0 ? EV::dfinfo_t::NOPORT : 0) | EV::dfinfo_t::DN,
                           e_.mic     = Base::lsbf4(&d[pend]),
                           e_.hdr     = d[LORA::OFF_DAT_HDR],
                           e_.fct     = d[LORA::OFF_DAT_FCT],
                           e_.port    = port,
                           e_.plen    = dlen,
                           e_.opts.length = olen,
                           memcpy(&e_.opts[0], opts, olen)));
    } else {
        EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_REPLAY,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = Base::lsbf4(&d[pend]),
                            e_.info2  = seqno));
        // discard the data
        LMICOS_logEventUint32("decodeFrame: discarding replay", ((u4_t)seqno << 16) | (LMIC.lastDnConf << 8) | (ftype << 0));
        goto norx;
    }

    if( // NWK acks but we don't have a frame pending
        (ackup && LMIC.txCnt == 0) ||
        // We sent up confirmed and we got a response in DNW1/DNW2
        // BUT it did not carry an ACK - this should never happen
        // Do not resend and assume frame was not ACKed.
        (!ackup && LMIC.txCnt != 0) ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::SPURIOUS_ACK,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = seqno,
                           e_.info2  = ackup));
#if LMIC_DEBUG_LEVEL > 1
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": ??ack error ack=%d txCnt=%d\n", os_getTime(), ackup, LMIC.txCnt);
#endif
    }

    if( LMIC.txCnt != 0 ) // we requested an ACK
        orTxrxFlags(__func__, ackup ? TXRX_ACK : TXRX_NACK);

    if( port <= 0 ) {
        orTxrxFlags(__func__, TXRX_NOPORT);
        LMIC.dataBeg = poff;
        LMIC.dataLen = 0;
    } else {
        orTxrxFlags(__func__, TXRX_PORT);
        LMIC.dataBeg = poff;
        LMIC.dataLen = pend-poff;
    }
#if LMIC_DEBUG_LEVEL > 0
    LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": Received downlink, window=%s, port=%d, ack=%d, txrxFlags=%#x\n", os_getTime(), window, port, ackup, LMIC.txrxFlags);
#endif
    return 1;
}


// ================================================================================
// TX/RX transaction support

// start reception and log.
static void radioRx (void) {
    reportEventNoUpdate(EV_RXSTART);
    os_radio(RADIO_RX);
}

// start RX in window 2.
static void setupRx2 (void) {
    initTxrxFlags(__func__, TXRX_DNW2);
    LMIC.rps = dndr2rps(LMIC.dn2Dr);
    LMIC.freq = LMIC.dn2Freq;
    LMIC.dataLen = 0;
    radioRx();
}

//! \brief Adjust the delay (in ticks) of the target window-open time from nominal.
//! \param hsym the duration of one-half symbol in osticks.
//! \param rxsyms_in the nominal window length -- minimum length of time to delay.
//! \return Effective delay to use (positive for later, negative for earlier).
//! \post LMIC.rxsyms is set to the number of rxsymbols to be used for preamble timeout.
//! \bug For FSK, the radio driver ignores LMIC.rxsysms, and uses a fixed value of 4080 bits
//! (81 ms)
//!
//! \details The calculation of the RX Window opening time has to balance several things.
//! The system clock might be inaccurate. Generally, the LMIC assumes that the timebase
//! is accurage to 100 ppm, or 0.01%.  0.01% of a 6 second window is 600 microseconds.
//! For LoRa, the fastest data rates of interest is SF7 (1024 us/symbol); with an 8-byte
//! preamble, the shortest preamble is 8.092ms long. If using FSK, the symbol rate is
//! 20 microseconds, but the preamble is 8*5 bits long, so the preamble is 800 microseconds.
//! Unless LMIC_ENABLE_arbitrary_clock_error is true, we fold clock errors of > 0.4% back
//! to 0.4%.
ostime_t LMICcore_adjustForDrift (ostime_t delay, ostime_t hsym, rxsyms_t rxsyms_in) {
    ostime_t rxoffset;

    // decide if we want to move left or right of the reference time.
    rxoffset = -LMICbandplan_RX_EXTRA_MARGIN_osticks;

    u2_t clockerr = LMIC.client.clockError;

    // Most crystal oscillators are 100 ppm. If things are that tight, there's
    // no point in specifying a drift, as 6 seconds at 100ppm is +/- 600 microseconds.
    // We position the windows at the front, and there's some extra margin, so...
    // don't bother setting values <= 100 ppm.
    if (clockerr != 0)
        {
        // client has set clock error. Limit this to 0.1% unless there's
        // a compile-time configuration. (In other words, assume that millis()
        // clock is accurate to 0.1%.) You should never use clockerror to
        // compensate for system-late problems.
        u2_t const maxError = LMIC_kMaxClockError_ppm * MAX_CLOCK_ERROR / (1000 * 1000);
        if (! LMIC_ENABLE_arbitrary_clock_error && clockerr > maxError)
            {
            clockerr = maxError;
            }
        }

    // If the clock is slow, the window needs to open earlier in our time
    // in order to open at or before the specified time (in real world),.
    // Don't bother to round, as this is very fine-grained.
    ostime_t drift = (ostime_t)(((int64_t)delay * clockerr) / MAX_CLOCK_ERROR);

    // calculate the additional rxsyms needed to hit the window nominally.
    ostime_t const tsym = 2 * hsym;
    ostime_t driftwin;
    driftwin = 2 * drift;
    if (rxoffset < 0)
        driftwin += -rxoffset;
    // else we'll hit the window nominally.

    rxsyms_in += (driftwin + tsym - 1) / tsym;

    // reduce the rxoffset by the drift; this compensates for a slow clock;
    // but it makes the rxtime too early by approximately `drift` if clock
    // is fast.
    rxoffset -= drift;

    setRxsyms(rxsyms_in);

    return delay + rxoffset;
}

static void schedRx12 (ostime_t delay, osjobcb_t func, u1_t dr) {
    ostime_t hsym = dr2hsym(dr);

    // Schedule the start of the receive window. os_getRadioRxRampup() is used to make sure we
    // exit "sleep" well enough in advance of the receive window to be able to
    // time things accurately.
    //
    // This also sets LMIC.rxsyms. This is NOT normally used for FSK; see LMICbandplan_txDoneFSK()
    LMIC.rxtime = LMIC.txend + LMICcore_adjustForDrift(delay, hsym, LMICbandplan_MINRX_SYMS_LoRa_ClassA);

    LMIC_X_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": sched Rx12 %"LMIC_PRId_ostime_t"\n", os_getTime(), LMIC.rxtime - os_getRadioRxRampup());
    os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - os_getRadioRxRampup(), func);
}

static void setupRx1 (osjobcb_t func) {
    initTxrxFlags(__func__, TXRX_DNW1);
    // Turn LMIC.rps from TX over to RX
    LMIC.rps = setNocrc(LMIC.rps,1);
    LMIC.dataLen = 0;
    LMIC.osjob.func = func;
    radioRx();
}


// Called by HAL once TX complete and delivers exact end of TX time stamp in LMIC.rxtime
static void txDone (ostime_t delay, osjobcb_t func) {
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & (OP_TRACK|OP_PINGABLE|OP_PINGINI)) == (OP_TRACK|OP_PINGABLE) ) {
        rxschedInit(&LMIC.ping);    // note: reuses LMIC.frame buffer!
        LMIC.opmode |= OP_PINGINI;
    }
#endif // !DISABLE_PING

    // Change RX frequency (can happen even for EU-like if programmed by DlChannelReq)
    // change params and rps (US only) before we increment txChnl
    LMICbandplan_setRx1Params();

    // LMIC.dndr carries the TX datarate (can be != LMIC.datarate [confirm retries etc.])
    // Setup receive -- either schedule FSK or schedule rx1 or rx2 window.
    if( LMICbandplan_isFSK() ) {
        LMICbandplan_txDoneFSK(delay, func);
    }
    else
    {
        schedRx12(delay, func, LMIC.dndr);
    }
}

// ======================================== Join frames


#if !defined(DISABLE_JOIN)
static void onJoinFailed (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    // Notify app - must call LMIC_reset() to stop joining
    // otherwise join procedure continues.
    reportEventAndUpdate(EV_JOIN_FAILED);
}

// process join-accept message or deal with no join-accept in slot 2.
static bit_t processJoinAccept (void) {
    if ((LMIC.txrxFlags & TXRX_DNW1) != 0 && LMIC.dataLen == 0)
        return 0;

    // formerly we asserted.
    if ((LMIC.opmode & OP_TXRXPEND) == 0)
        // nothing we can do.
        return 1;

    // formerly we asserted.
    if ((LMIC.opmode & (OP_JOINING|OP_REJOIN)) == 0) {
        // we shouldn't be here. just drop the frame, but clean up txrxpend.
        return processJoinAccept_badframe();
    }

    if( LMIC.dataLen == 0 ) {
        // we didn't get any data and we're in slot 2. So... there's no join frame.
        return processJoinAccept_nojoinframe();
    }

    u1_t hdr  = LMIC.frame[0];
    u1_t dlen = LMIC.dataLen;
    u4_t mic  = os_rlsbf4(&LMIC.frame[dlen-4]); // safe before modified by encrypt!
    LMIC_EV_VARIABLE(mic);                      // only used by EV().

    if( (dlen != LEN_JA && dlen != LEN_JAEXT)
        || (hdr & (HDR_FTYPE|HDR_MAJOR)) != (HDR_FTYPE_JACC|HDR_MAJOR_V1) ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::UNEXPECTED_FRAME,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = dlen < 4 ? 0 : mic,
                           e_.info2  = hdr + (dlen<<8)));
        return processJoinAccept_badframe();
    }
    aes_encrypt(LMIC.frame+1, dlen-1);
    if( !aes_verifyMic0(LMIC.frame, dlen-4) ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::JOIN_BAD_MIC,
                           e_.info   = mic));
        return processJoinAccept_badframe();
    }

    u4_t addr = os_rlsbf4(LMIC.frame+OFF_JA_DEVADDR);
    LMIC.devaddr = addr;
    LMIC.netid = os_rlsbf4(&LMIC.frame[OFF_JA_NETID]) & 0xFFFFFF;

    // initDefaultChannels(0) for EU-like, nothing otherwise
    LMICbandplan_joinAcceptChannelClear();

    if (!LMICbandplan_hasJoinCFlist() && dlen > LEN_JA) {
            // if no JoinCFList, we're supposed to continue
            // the join per 2.2.5 of LoRaWAN regional 2.2.4
            // https://github.com/mcci-catena/arduino-lmic/issues/19
    } else if ( LMICbandplan_hasJoinCFlist() && dlen > LEN_JA ) {
        dlen = OFF_CFLIST;
        for( u1_t chidx=3; chidx<8; chidx++, dlen+=3 ) {
            u4_t freq = LMICbandplan_convFreq(&LMIC.frame[dlen]);
            if( freq ) {
                LMIC_setupChannel(chidx, freq, 0, -1);
#if LMIC_DEBUG_LEVEL > 1
                LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": Setup channel, idx=%d, freq=%"PRIu32"\n", os_getTime(), chidx, freq);
#endif
            }
        }
    }

    // already incremented when JOIN REQ got sent off
    aes_sessKeys(LMIC.devNonce-1, &LMIC.frame[OFF_JA_ARTNONCE], LMIC.nwkKey, LMIC.artKey);
    DO_DEVDB(LMIC.netid,   netid);
    DO_DEVDB(LMIC.devaddr, devaddr);
    DO_DEVDB(LMIC.nwkKey,  nwkkey);
    DO_DEVDB(LMIC.artKey,  artkey);

    EV(joininfo, INFO, (e_.arteui  = MAIN::CDEV->getArtEui(),
                        e_.deveui  = MAIN::CDEV->getEui(),
                        e_.devaddr = LMIC.devaddr,
                        e_.oldaddr = oldaddr,
                        e_.nonce   = LMIC.devNonce-1,
                        e_.mic     = mic,
                        e_.reason  = ((LMIC.opmode & OP_REJOIN) != 0
                                      ? EV::joininfo_t::REJOIN_ACCEPT
                                      : EV::joininfo_t::ACCEPT)));

    //
    // XXX(tmm@mcci.com) OP_REJOIN confuses me, and I'm not sure why we're
    // adjusting DRs here. We've just received a join accept, and the
    // datarate therefore shouldn't be in play.  In effect, we set the
    // initial data rate based on the number of times we tried to rejoin.
    //
    if( (LMIC.opmode & OP_REJOIN) != 0 ) {
#if CFG_region != LMIC_REGION_as923
	    // TODO(tmm@mcci.com) regionalize
        // Lower DR every try below current UP DR
        // need to check feasibility? join feasability is default.
        LMIC.datarate = lowerDR(LMIC.datarate, LMIC.rejoinCnt);
#else
        // in the join of AS923 v1.1 or older, only DR2 (SF10) is used.
        // TODO(tmm@mcci.com) if the rejoin logic is at all correct, we
        // should be setting the uplink datarate based on the number of
        // tries; this doesn't set the AS923 join data rate.
        LMIC.datarate = AS923_DR_SF10;
#endif
    }
    LMIC.opmode &= ~(OP_JOINING|OP_TRACK|OP_REJOIN|OP_TXRXPEND|OP_PINGINI);
    LMIC.opmode |= OP_NEXTCHNL;
    LMIC.txCnt = 0;
    stateJustJoined();
    // transition to the ADR_ACK initial state.
    setAdrAckCount(LINK_CHECK_INIT);

    LMIC.dn2Dr = LMIC.frame[OFF_JA_DLSET] & 0x0F;
    LMIC.rx1DrOffset = (LMIC.frame[OFF_JA_DLSET] >> 4) & 0x7;
    LMIC.rxDelay = LMIC.frame[OFF_JA_RXDLY];
    if (LMIC.rxDelay == 0) LMIC.rxDelay = 1;
    reportEventAndUpdate(EV_JOINED);
    return 1;
}

static bit_t processJoinAccept_badframe(void) {
        if( (LMIC.txrxFlags & TXRX_DNW1) != 0 )
            // continue the join process: there's another window.
            return 0;
        else
            // stop the join process
            return processJoinAccept_nojoinframe();
}

static bit_t processJoinAccept_nojoinframe(void) {
        // Valid states are JOINING (in which caise REJOIN is ignored)
        // or ~JOINING and REJOIN. If it's a REJOIN,
        // we need to turn off rejoin, signal an event, and increment
        // the rejoin-sent count. Internal callers will turn on rejoin
        // occasionally.
        if( (LMIC.opmode & OP_JOINING) == 0) {
            // formerly, we asserted ((LMIC.opmode & OP_REJOIN) != 0);
            // but now we just return 1 if it's not asserted.
            if ( (LMIC.opmode & OP_REJOIN) == 0) {
                LMIC.opmode &= ~OP_TXRXPEND;
                return 1;
            }
            LMIC.opmode &= ~(OP_REJOIN|OP_TXRXPEND);
            if( LMIC.rejoinCnt < 10 )
                LMIC.rejoinCnt++;
            reportEventAndUpdate(EV_REJOIN_FAILED);
            // stop the join process.
            return 1;
        }
        // otherwise it's a normal join. At end of rx2, so we
        // need to schedule something.
        LMIC.opmode &= ~OP_TXRXPEND;
        reportEventNoUpdate(EV_JOIN_TXCOMPLETE);
        int failed = LMICbandplan_nextJoinState();
        EV(devCond, DEBUG, (e_.reason = EV::devCond_t::NO_JACC,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = LMIC.datarate|DR_PAGE,
                            e_.info2  = failed));
        // Build next JOIN REQUEST with next engineUpdate call
        // Optionally, report join failed.
        // Both after a random/chosen amount of ticks. That time
        // is in LMIC.txend. The delay here is either zero or 1
        // tick; onJoinFailed()/runEngineUpdate() are responsible
        // for honoring that. XXX(tmm@mcci.com) The IBM 1.6 code
        // claimed to return a delay but really returns 0 or 1.
        // Once we update as923 to return failed after dr2, we
        // can take out this #if.
        os_setTimedCallback(&LMIC.osjob, os_getTime()+failed,
                            failed
                            ? FUNC_ADDR(onJoinFailed)      // one JOIN iteration done and failed
                            : FUNC_ADDR(runEngineUpdate)); // next step to be delayed
        // stop this join process.
        return 1;
}

static void processRx2Jacc (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    if( LMIC.dataLen == 0 ) {
        initTxrxFlags(__func__, 0);  // nothing in 1st/2nd DN slot
    }
    // we're done with this join cycle anyway, so ignore the
    // result of processJoinAccept()
    (void) processJoinAccept();
}


static void setupRx2Jacc (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    LMIC.osjob.func = FUNC_ADDR(processRx2Jacc);
    setupRx2();
}


static void processRx1Jacc (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    if( LMIC.dataLen == 0 || !processJoinAccept() )
        schedRx12(DELAY_JACC2_osticks, FUNC_ADDR(setupRx2Jacc), LMIC.dn2Dr);
}


static void setupRx1Jacc (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    setupRx1(FUNC_ADDR(processRx1Jacc));
}


static void jreqDone (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    txDone(DELAY_JACC1_osticks, FUNC_ADDR(setupRx1Jacc));
}

#endif // !DISABLE_JOIN

// ======================================== Data frames

// Fwd decl.
static bit_t processDnData(void);

static void processRx2DnData (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    if( LMIC.dataLen == 0 ) {
        initTxrxFlags(__func__, 0);  // nothing in 1st/2nd DN slot
        // It could be that the gateway *is* sending a reply, but we
        // just didn't pick it up. To avoid TX'ing again while the
        // gateay is not listening anyway, delay the next transmission
        // until DNW2_SAFETY_ZONE from now, and add up to 2 seconds of
        // extra randomization.
        // BUG(tmm@mcci.com) this delay is not needed for some
        // regions, e.g. US915 and AU915, which have non-overlapping
        // uplink and downlink.
        txDelay(os_getTime() + DNW2_SAFETY_ZONE, 2);
    }
    processDnData();
}


static void setupRx2DnData (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    LMIC.osjob.func = FUNC_ADDR(processRx2DnData);
    setupRx2();
}


static void processRx1DnData (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    if( LMIC.dataLen == 0 || !processDnData() )
        schedRx12(sec2osticks(LMIC.rxDelay +(int)DELAY_EXTDNW2), FUNC_ADDR(setupRx2DnData), LMIC.dn2Dr);
}


static void setupRx1DnData (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    setupRx1(FUNC_ADDR(processRx1DnData));
}


static void updataDone (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    txDone(sec2osticks(LMIC.rxDelay), FUNC_ADDR(setupRx1DnData));
}

// ========================================

static bit_t sendAdrAckReq(void) {
    if (LMIC.adrAckReq < LINK_CHECK_CONT) {
        return 0;
    } else if (LMIC.adrAckReq <= LINK_CHECK_DEAD) {
        return 1;
    } else if (LMIC.adrAckReq <= LINK_CHECK_DEAD + 32) {
        // for compliance, though it's not clear why they care, we stop sending requests
        // when we're right at the DEAD state
        return 0;
    } else if (LMIC.adrAckReq <= LINK_CHECK_UNJOIN - 32) {
        return 0;
    } else {
        // otherwise, if our alternative is to unjoin and we have no other info, keep
        // asking for a downlink.
        return 1;
    }
}

static bit_t buildDataFrame (void) {
    bit_t txdata = ((LMIC.opmode & (OP_TXDATA|OP_POLL)) != OP_POLL);
    u1_t dlen = txdata ? LMIC.pendTxLen : 0;

    // Piggyback MAC options
    // Prioritize by importance
    // highest importance are the ones in the pendMac buffer.
    int  end = OFF_DAT_OPTS;

    // Send piggyback data if: !txdata or txport != 0
    if ((! txdata || LMIC.pendTxPort != 0) && LMIC.pendMacPiggyback && LMIC.pendMacLen != 0) {
        os_copyMem(LMIC.frame + end, LMIC.pendMacData, LMIC.pendMacLen);
        end += LMIC.pendMacLen;
    }
    LMIC.pendMacLen = 0;
    LMIC.pendMacPiggyback = 0;

#if !defined(DISABLE_MCMD_RXParamSetupReq)
    // per 5.4, RxParamSetupAns is sticky.
    if (LMIC.dn2Ans) {
        if (LMIC.dn2Ans & 0x40) {
            LMIC.dn2Ans ^= 0x40;
        } else {
            LMIC.frame[end + 0] = MCMD_RXParamSetupAns;
            LMIC.frame[end + 1] = LMIC.dn2Ans & ~MCMD_RXParamSetupAns_RFU;
            end += 2;
        }
    }
#endif // !DISABLE_MCMD_RXParamSetupReq
#if !defined(DISABLE_MCMD_DlChannelReq)
    // per 5.4, DlChannelAns is sticky.
    if (LMIC.macDlChannelAns) {
        if (LMIC.macDlChannelAns & 0x40) {
            LMIC.macDlChannelAns ^= 0x40;
        } else {
            LMIC.frame[end + 0] = MCMD_DlChannelAns;
            LMIC.frame[end + 1] = LMIC.macDlChannelAns & ~MCMD_DlChannelAns_RFU;
            end += 2;
        }
    }
#endif // !DISABLE_MCMD_DlChannelReq
#if !defined(DISABLE_MCMD_RXTimingSetupReq)
    // per 5.7, RXTimingSetupAns is sticky
    if (LMIC.macRxTimingSetupAns == 2) {
        LMIC.macRxTimingSetupAns = 1;
    } else if (LMIC.macRxTimingSetupAns) {
        LMIC.frame[end++] = MCMD_RXTimingSetupAns;
    }
#endif // !DISABLE_MCMD_RXTimingSetupReq)

#if LMIC_ENABLE_DeviceTimeReq
    if ( LMIC.txDeviceTimeReqState == lmic_RequestTimeState_tx ) {
        LMIC.frame[end+0] = MCMD_DeviceTimeReq;
        end += 1;
        LMIC.txDeviceTimeReqState = lmic_RequestTimeState_rx;
    }
#endif // LMIC_ENABLE_DeviceTimeReq
#if !defined(DISABLE_BEACONS) && defined(ENABLE_MCMD_BeaconTimingAns)
    if ( LMIC.bcninfoTries > 0 ) {
        LMIC.frame[end+0] = MCMD_BeaconInfoReq;
        end += 1;
    }
#endif
    if (end > OFF_DAT_OPTS + 16) {
        LMICOS_logEventUint32("piggyback mac opts too long", end);
        return 0;
    }

    if( LMIC.adrChanged ) {
        // if ADR is enabled, and we were just counting down the
        // transmits before starting an ADR, advance the timer so
        // we'll do an ADR now.
        if (LMIC.adrAckReq < LINK_CHECK_CONT)
            setAdrAckCount(LINK_CHECK_CONT);
        LMIC.adrChanged = 0;
    }

    unsigned int flen = end + (txdata ? 5+dlen : 4);
    if( flen > MAX_LEN_FRAME ) {
        // Options and payload too big - delay payload
        txdata = 0;
        flen = end+4;
    }

    u1_t maxFlen = LMICbandplan_maxFrameLen(LMIC.datarate);

    if (flen > maxFlen) {
        LMICOS_logEventUint32("frame too long for this bandplan", ((u4_t)dlen << 16) | (flen << 8) | maxFlen);
        return 0;
    }

    LMIC.frame[OFF_DAT_HDR] = HDR_FTYPE_DAUP | HDR_MAJOR_V1;
    LMIC.frame[OFF_DAT_FCT] = (LMIC.dnConf | LMIC.adrEnabled
                              | (sendAdrAckReq() ? FCT_ADRACKReq : 0)
                              | (end-OFF_DAT_OPTS));
    os_wlsbf4(LMIC.frame+OFF_DAT_ADDR,  LMIC.devaddr);

    if( LMIC.txCnt == 0 && LMIC.upRepeatCount == 0 ) {
        LMIC.seqnoUp += 1;
        DO_DEVDB(LMIC.seqnoUp,seqnoUp);
    } else {
        LMICOS_logEventUint32("retransmit", ((u4_t)LMIC.frame[OFF_DAT_FCT] << 24u) | ((u4_t)LMIC.txCnt << 16u) | (LMIC.upRepeatCount << 8u) | (LMIC.upRepeat<<0u));
        EV(devCond, INFO, (e_.reason = EV::devCond_t::RE_TX,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = LMIC.seqnoUp-1,
                           e_.info2  = ((LMIC.txCnt+1) |
                                        (LMIC.upRepeatCount << 8) |
                                        ((LMIC.datarate|DR_PAGE)<<16))));
    }
    os_wlsbf2(LMIC.frame+OFF_DAT_SEQNO, LMIC.seqnoUp-1);

    // Clear pending DN confirmation
    LMIC.dnConf = 0;

    if( txdata ) {
        if( LMIC.pendTxConf ) {
            // Confirmed only makes sense if we have a payload (or at least a port)
            LMIC.frame[OFF_DAT_HDR] = HDR_FTYPE_DCUP | HDR_MAJOR_V1;
            if( LMIC.txCnt == 0 ) LMIC.txCnt = 1;
        } else if (LMIC.upRepeat != 0) {
            // we are repeating.  So we need to count here.
            if (LMIC.upRepeatCount == 0) {
                LMIC.upRepeatCount = 1;
            }
        }
        LMIC.frame[end] = LMIC.pendTxPort;
        os_copyMem(LMIC.frame+end+1, LMIC.pendTxData, dlen);
        aes_cipher(LMIC.pendTxPort==0 ? LMIC.nwkKey : LMIC.artKey,
                   LMIC.devaddr, LMIC.seqnoUp-1,
                   /*up*/0, LMIC.frame+end+1, dlen);
    }
    aes_appendMic(LMIC.nwkKey, LMIC.devaddr, LMIC.seqnoUp-1, /*up*/0, LMIC.frame, flen-4);

    EV(dfinfo, DEBUG, (e_.deveui  = MAIN::CDEV->getEui(),
                       e_.devaddr = LMIC.devaddr,
                       e_.seqno   = LMIC.seqnoUp-1,
                       e_.flags   = (LMIC.pendTxPort < 0 ? EV::dfinfo_t::NOPORT : EV::dfinfo_t::NOP),
                       e_.mic     = Base::lsbf4(&LMIC.frame[flen-4]),
                       e_.hdr     = LMIC.frame[LORA::OFF_DAT_HDR],
                       e_.fct     = LMIC.frame[LORA::OFF_DAT_FCT],
                       e_.port    = LMIC.pendTxPort,
                       e_.plen    = txdata ? dlen : 0,
                       e_.opts.length = end-LORA::OFF_DAT_OPTS,
                       memcpy(&e_.opts[0], LMIC.frame+LORA::OFF_DAT_OPTS, end-LORA::OFF_DAT_OPTS)));
    LMIC.dataLen = flen;
    return 1;
}


#if !defined(DISABLE_BEACONS)
// Callback from HAL during scan mode or when job timer expires.
static void onBcnRx (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    // If we arrive via job timer make sure to put radio to rest.
    os_radio(RADIO_RST);
    os_clearCallback(&LMIC.osjob);
    if( LMIC.dataLen == 0 ) {
        // Nothing received - timeout
        LMIC.opmode &= ~(OP_SCAN | OP_TRACK);
        reportEventAndUpdate(EV_SCAN_TIMEOUT);
        return;
    }
    if( ! LMIC_BEACON_SUCCESSFUL(decodeBeacon()) ) {
        // Something is wrong with the beacon - continue scan
        LMIC.dataLen = 0;
        os_radio(RADIO_RXON);
        os_setTimedCallback(&LMIC.osjob, LMIC.bcninfo.txtime, FUNC_ADDR(onBcnRx));
        return;
    }
    // Found our 1st beacon
    // We don't have a previous beacon to calc some drift - assume
    // an max error of 13ms = 128sec*100ppm which is roughly +/-100ppm
    calcBcnRxWindowFromMillis(13,1);
    LMIC.opmode &= ~OP_SCAN;          // turn SCAN off
    LMIC.opmode |=  OP_TRACK;         // auto enable tracking
    reportEventAndUpdate(EV_BEACON_FOUND);    // can be disabled in callback
}


// Enable receiver to listen to incoming beacons
// netid defines when scan stops (any or specific beacon)
// This mode ends with events: EV_SCAN_TIMEOUT/EV_SCAN_BEACON
// Implicitely cancels any pending TX/RX transaction.
// Also cancels an onpoing joining procedure.
static void startScan (void) {
    // formerly, we asserted.
    if (LMIC.devaddr == 0 || (LMIC.opmode & OP_JOINING) != 0)
        return;
    if( (LMIC.opmode & OP_SHUTDOWN) != 0 )
        return;
    // Cancel onging TX/RX transaction
    LMIC.txCnt = LMIC.dnConf = LMIC.bcninfo.flags = 0;
    LMIC.opmode = (LMIC.opmode | OP_SCAN) & ~(OP_TXRXPEND);
    LMICbandplan_setBcnRxParams();
    LMIC.rxtime = LMIC.bcninfo.txtime = os_getTime() + sec2osticks(BCN_INTV_sec+1);
    os_setTimedCallback(&LMIC.osjob, LMIC.rxtime, FUNC_ADDR(onBcnRx));
    os_radio(RADIO_RXON);
}


bit_t LMIC_enableTracking (u1_t tryBcnInfo) {
    if( (LMIC.opmode & (OP_SCAN|OP_TRACK|OP_SHUTDOWN)) != 0 )
        return 0;  // already in progress or failed to enable
    // If BCN info requested from NWK then app has to take are
    // of sending data up so that MCMD_BeaconInfoReq can be attached.
    if( (LMIC.bcninfoTries = tryBcnInfo) == 0 )
        startScan();
    return 1;  // enabled
}


void LMIC_disableTracking (void) {
    LMIC.opmode &= ~(OP_SCAN|OP_TRACK);
    LMIC.bcninfoTries = 0;
    engineUpdate();
}
#endif // !DISABLE_BEACONS



























// ================================================================================
//
// Join stuff
//
// ================================================================================

#if !defined(DISABLE_JOIN)
static void buildJoinRequest (u1_t ftype) {
    // Do not use pendTxData since we might have a pending
    // user level frame in there. Use RX holding area instead.
    xref2u1_t d = LMIC.frame;
    d[OFF_JR_HDR] = ftype;
    os_getArtEui(d + OFF_JR_ARTEUI);
    os_getDevEui(d + OFF_JR_DEVEUI);
    os_wlsbf2(d + OFF_JR_DEVNONCE, LMIC.devNonce);
    aes_appendMic0(d, OFF_JR_MIC);

    EV(joininfo,INFO,(e_.deveui  = MAIN::CDEV->getEui(),
                      e_.arteui  = MAIN::CDEV->getArtEui(),
                      e_.nonce   = LMIC.devNonce,
                      e_.oldaddr = LMIC.devaddr,
                      e_.mic     = Base::lsbf4(&d[LORA::OFF_JR_MIC]),
                      e_.reason  = ((LMIC.opmode & OP_REJOIN) != 0
                                    ? EV::joininfo_t::REJOIN_REQUEST
                                    : EV::joininfo_t::REQUEST)));
    LMIC.dataLen = LEN_JR;
    LMIC.devNonce++;
    DO_DEVDB(LMIC.devNonce,devNonce);
}

static void startJoining (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    // see issue #244: for backwards compatibility
    // don't override what the user does after os_init().
    if (LMIC.initBandplanAfterReset)
        LMICbandplan_resetDefaultChannels();
    else
        LMIC.initBandplanAfterReset = 1;

    // let the client know that now's the time to update
    // network settings.
    reportEventAndUpdate(EV_JOINING);
}

// reset the joined-to-network state (and clean up)
void LMIC_unjoin(void) {
    // reset any joining flags
    LMIC.opmode &= ~(OP_SCAN|OP_REJOIN|OP_UNJOIN);

    // put us in unjoined state:
    LMIC.devaddr = 0;

    // clear transmit.
    LMIC_clrTxData();
}

// Start join procedure if not already joined.
bit_t LMIC_startJoining (void) {
    if( LMIC.devaddr == 0 ) {
        // There should be no TX/RX going on
        // ASSERT((LMIC.opmode & (OP_POLL|OP_TXRXPEND)) == 0);
        LMIC.opmode &= ~OP_POLL;
        // Lift any previous duty limitation
        LMIC.globalDutyRate = 0;
        // Cancel scanning
        LMIC.opmode &= ~(OP_SCAN|OP_UNJOIN|OP_REJOIN|OP_LINKDEAD|OP_NEXTCHNL);
        // Setup state
        LMIC.rejoinCnt = LMIC.txCnt = 0;
        resetJoinParams();
        LMICbandplan_initJoinLoop();
        LMIC.opmode |= OP_JOINING;
        // reportEventAndUpdate will call engineUpdate which then starts sending JOIN REQUESTS
        os_setCallback(&LMIC.osjob, FUNC_ADDR(startJoining));
        return 1;
    }
    return 0; // already joined
}

static void unjoinAndRejoin(xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);
    LMIC_unjoin();
    LMIC_startJoining();
}

// do a deferred unjoin and rejoin, so not in engineupdate.
void LMIC_unjoinAndRejoin(void) {
    os_setCallback(&LMIC.osjob, FUNC_ADDR(unjoinAndRejoin));
}

#endif // !DISABLE_JOIN


// ================================================================================
//
//
//
// ================================================================================

#if !defined(DISABLE_PING)
static void processPingRx (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    if( LMIC.dataLen != 0 ) {
        initTxrxFlags(__func__, TXRX_PING);
        if( decodeFrame() ) {
            reportEventNoUpdate(EV_RXCOMPLETE);
        }
    }
    // Pick next ping slot
    engineUpdate();
}
#endif // !DISABLE_PING

// process downlink data at close of RX window.  Return zero if another RX window
// should be scheduled, non-zero to prevent scheduling of RX2 (if relevant).
// Confusingly, the caller actualyl does some of the calculation, so the answer from
// us is not always totaly right; the rx1 window check ignores our result unless
// LMIC.datalen was non zero before calling.
//
// Inputs:
//  LMIC.dataLen    number of bytes receieved; 0 --> no message at all received.
//  LMIC.txCnt      currnt confirmed uplink count, or 0 for unconfirmed.
//  LMIC.txrxflags  state of play for the Class A engine and message receipt.
//
// and many other flags in txcomplete().

// forward references.
static bit_t processDnData_norx(void);
static bit_t processDnData_txcomplete(void);

static bit_t processDnData (void) {
    // if no TXRXPEND, we shouldn't be here and can do nothign.
    // formerly we asserted.
    if ((LMIC.opmode & OP_TXRXPEND) == 0)
        return 1;

    if( LMIC.dataLen == 0 ) {
        // if this is an RX1 window, shouldn't we return 0 to schedule
        // RX2?  in fact, the rx1 caller ignores what we return, and
        // norx() doesn't call txcomplete if this is RX1.
        return processDnData_norx();
    }
    // if we get here, LMIC.dataLen != 0, so there is some
    // traffic.
    else if( !decodeFrame() ) {
        // if we are in downlink window 1, we need to schedule
        // downlink window 2.
        if( (LMIC.txrxFlags & TXRX_DNW1) != 0 )
            return 0;
        else
            // otherwise we are in downlink window 2; we will not
            // get any more downlink traffic from this uplink, so we need
            // to close the books on this uplink attempt
            return processDnData_norx();
    }
    // downlink frame was accepted. This means that we're done. Except
    // there's one bizarre corner case. If we sent a confirmed message
    // and got a downlink that didn't have an ACK, we have to retry.
    // It is not clear why the network is permitted to do this; the
    // fact that they scheduled a downlink for us during one of the RX
    // windows is clear confirmation that the uplink made it to the
    // network and was valid. However, compliance checks this, so
    // we have to handle it and retransmit.
    else if (LMIC.txCnt != 0 && (LMIC.txrxFlags & TXRX_NACK) != 0)
        {
        // grr.  we're confirmed but the network downlink did not
        // set the ACK bit. We know txCnt is non-zero, so this
        // will immediately fall into the retransmit path. We don't
        // want to do this unless it's a confirmed uplink.
        return processDnData_norx();
        }
    // the transmit of the uplink is really complete.
    else {
        return processDnData_txcomplete();
    }
}

// nothing was received this window.
static bit_t processDnData_norx(void) {
    if( LMIC.txCnt != 0 ) {
        if( LMIC.txCnt < TXCONF_ATTEMPTS ) {
            // Per [1.0.3] section 18.4, it is recommended that the device adjust datarate down.
            // The spec is not clear about what should happen in case the data size is too large
            // for the new frame len, but it seems that we should leave theframe len at the new
            // data size. Therefore, we set the new data rate here, and then check at transmit time
            // whether the packet is now too large; if so, we abandon the transmission.
            LMIC.txCnt += 1;
            // becase txCnt was at least 1 when we entered this branch, this if() will be taken
            // for txCnt == 3, 5, 7.
            if (LMIC.txCnt & 1) {
                dr_t adjustedDR;
                // lower DR
                adjustedDR = decDR(LMIC.datarate);
                setDrTxpow(DRCHG_NOACK, adjustedDR, KEEP_TXPOW);
            }

            // TODO(tmm@mcci.com): check feasibility of lower datarate
            // Schedule another retransmission
            txDelay(LMIC.rxtime, RETRY_PERIOD_secs);
            LMIC.opmode &= ~OP_TXRXPEND;
            engineUpdate();
            return 1;
        }
        // confirmed uplink is complete without an ack: no port and no flag
        initTxrxFlags(__func__, TXRX_NACK | TXRX_NOPORT);
    } else if (LMIC.upRepeatCount != 0) {
        if (LMIC.upRepeatCount < LMIC.upRepeat) {
            LMICOS_logEventUint32("processDnData: repeat", (LMIC.upRepeat<<8u) | (LMIC.upRepeatCount<<0u));
            LMIC.upRepeatCount += 1;
            txDelay(os_getTime() + ms2osticks(LMICbandplan_TX_RECOVERY_ms), 0);
            LMIC.opmode &= ~OP_TXRXPEND;
            engineUpdate();
            return 1;
        }
        // counted out: nothing received.
        initTxrxFlags(__func__, TXRX_NOPORT);
    } else {
        // Nothing received - implies no port
        initTxrxFlags(__func__, TXRX_NOPORT);
    }
    setAdrAckCount(LMIC.adrAckReq + 1);
    LMIC.dataBeg = LMIC.dataLen = 0;

    return processDnData_txcomplete();
}

// this Class-A uplink-and-receive cycle is complete.
static bit_t processDnData_txcomplete(void) {
    LMIC.opmode &= ~(OP_TXDATA|OP_TXRXPEND);
    // turn off all the repeat stuff.
    LMIC.txCnt = LMIC.upRepeatCount = 0;

    // if there's pending mac data that's not piggyback, launch it now.
    if (LMIC.pendMacLen != 0) {
        if (LMIC.pendMacPiggyback) {
            LMICOS_logEvent("piggyback mac message");
            LMIC.opmode |= OP_POLL;     // send back the mac answers even if there's no data.
        } else {
            // Every mac command on port 0 requires an uplink, if there's data.
            // TODO(tmm@mcci.com) -- this is why we need a queueing structure for
            // uplinks.
            // open code the logic to build this because we don't want to call
            // engineUpdate right now.  Data is already in the uplink buffer.
            LMIC.pendTxConf = 0; // not confirmed
            LMIC.pendTxPort = 0; // port 0
            LMIC.pendTxLen  = LMIC.pendMacLen;
            LMIC.pendMacLen = 0; // discard mac data!
            LMIC.opmode |= OP_TXDATA;
            LMICOS_logEvent("port0 mac message");
        }
    }

    // Half-duplex gateways can have appreciable turn-around times,
    // so we force a wait. It might be nice to randomize this a little,
    // so that armies of identical devices will not try to talk all
    // at once. This is potentially band-specific, so we let it come
    // from the band-plan files.
    txDelay(os_getTime() + ms2osticks(LMICbandplan_TX_RECOVERY_ms), 0);

#if LMIC_ENABLE_DeviceTimeReq
    //
    // if the DeviceTimeReq FSM is active, we need to move it to idle,
    // completing the callback.
    //
    lmic_request_time_state_t const requestTimeState = LMIC.txDeviceTimeReqState;
    if ( requestTimeState != lmic_RequestTimeState_idle ) {
        lmic_request_network_time_cb_t * const pNetworkTimeCb = LMIC.client.pNetworkTimeCb;
        int flagSuccess = (LMIC.txDeviceTimeReqState == lmic_RequestTimeState_success);
        LMIC.txDeviceTimeReqState = lmic_RequestTimeState_idle;
        if (pNetworkTimeCb != NULL) {
            // reset the callback, so that the user's routine
            // can post another request if desired.
            LMIC.client.pNetworkTimeCb = NULL;

            // call the user's notification routine.
            (*pNetworkTimeCb)(LMIC.client.pNetworkTimeUserData, flagSuccess);
        }
    }
#endif // LMIC_ENABLE_DeviceTimeReq

    if( (LMIC.txrxFlags & (TXRX_DNW1|TXRX_DNW2|TXRX_PING)) != 0  &&  (LMIC.opmode & OP_LINKDEAD) != 0 ) {
        LMIC.opmode &= ~OP_LINKDEAD;
        reportEventNoUpdate(EV_LINK_ALIVE);
    }
    reportEventAndUpdate(EV_TXCOMPLETE);
    // If we haven't heard from NWK in a while although we asked for a sign
    // assume link is dead - notify application and keep going
    if( LMIC.adrAckReq > LINK_CHECK_DEAD ) {
        // We haven't heard from NWK for some time although we
        // asked for a response for some time - assume we're disconnected. Lower DR one notch.
        EV(devCond, ERR, (e_.reason = EV::devCond_t::LINK_DEAD,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = LMIC.adrAckReq));
        dr_t newDr = decDR((dr_t)LMIC.datarate);
        // newDr must be feasible; there must be at least
        // one channel that supports the new datarate. If not, stay
        // at current datarate (which finalizes things).
        if (! LMICbandplan_isDataRateFeasible(newDr)) {
            LMICOS_logEventUint32("LINK_CHECK_DEAD, new DR not feasible", (newDr << 8) | LMIC.datarate);
            newDr = LMIC.datarate;
        }
        if( newDr == (dr_t)LMIC.datarate) {
            // We are already at the minimum datarate
            // if the link is already marked dead, we need to join.
#if !defined(DISABLE_JOIN)
            if ( LMIC.adrAckReq > LINK_CHECK_UNJOIN ) {
                LMIC.opmode |= OP_UNJOIN;
            }
#endif // !defined(DISABLE_JOIN)
        } else if (newDr == LORAWAN_DR0) {
            // the spec says: the ADRACKReq shall not be set if
            // the device uses its lowest available data rate.
            // (1.0.3, 4.3.1.1, line 458)
            // We let the count continue to increase.
        } else {
            // we successfully lowered the data rate...
            // reset so that we'll lower again after the next
            // 32 uplinks.
            setAdrAckCount(LINK_CHECK_CONT);
        }
        // Decrease DataRate and restore fullpower.
        setDrTxpow(DRCHG_NOADRACK, newDr, pow2dBm(0));

        // be careful only to report EV_LINK_DEAD once.
        u2_t old_opmode = LMIC.opmode;
        LMIC.opmode = old_opmode | OP_LINKDEAD;
        if (LMIC.opmode != old_opmode)
            reportEventNoUpdate(EV_LINK_DEAD); // update?
    }
#if !defined(DISABLE_BEACONS)
    // If this falls to zero the NWK did not answer our MCMD_BeaconInfoReq commands - try full scan
    if( LMIC.bcninfoTries > 0 ) {
        if( (LMIC.opmode & OP_TRACK) != 0 ) {
            reportEventNoUpdate(EV_BEACON_FOUND); // update?
            LMIC.bcninfoTries = 0;
        }
        else if( --LMIC.bcninfoTries == 0 ) {
            startScan();   // NWK did not answer - try scan
        }
    }
#endif // !DISABLE_BEACONS
    return 1;
}

#if !defined(DISABLE_BEACONS)
static void processBeacon (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    ostime_t lasttx = LMIC.bcninfo.txtime;   // save here - decodeBeacon might overwrite
    u1_t flags = LMIC.bcninfo.flags;
    ev_t ev;

    if( LMIC.dataLen != 0 && LMIC_BEACON_SUCCESSFUL(decodeBeacon()) ) {
        ev = EV_BEACON_TRACKED;
        if( (flags & (BCN_PARTIAL|BCN_FULL)) == 0 ) {
            // We don't have a previous beacon to calc some drift - assume
            // an max error of 13ms = 128sec*100ppm which is roughly +/-100ppm
            calcBcnRxWindowFromMillis(13,0);
            goto rev;
        }
        // We have a previous BEACON to calculate some drift
        s2_t drift = BCN_INTV_osticks - (LMIC.bcninfo.txtime - lasttx);
        if( LMIC.missedBcns > 0 ) {
            drift = LMIC.drift + (drift - LMIC.drift) / (LMIC.missedBcns+1);
        }
        if( (LMIC.bcninfo.flags & BCN_NODRIFT) == 0 ) {
            s2_t diff = LMIC.drift - drift;
            if( diff < 0 ) diff = -diff;
            LMIC.lastDriftDiff = diff;
            if( LMIC.maxDriftDiff < diff )
                LMIC.maxDriftDiff = diff;
            LMIC.bcninfo.flags &= ~BCN_NODDIFF;
        }
        LMIC.drift = drift;
        LMIC.missedBcns = LMIC.rejoinCnt = 0;
        LMIC.bcninfo.flags &= ~BCN_NODRIFT;
        EV(devCond,INFO,(e_.reason = EV::devCond_t::CLOCK_DRIFT,
                         e_.eui    = MAIN::CDEV->getEui(),
                         e_.info   = drift,
                         e_.info2  = /*occasion BEACON*/0));
        // formerly we'd assert on BCN_PARTIAL|BCN_FULL, but we can't get here if so
    } else {
        ev = EV_BEACON_MISSED;
        LMIC.bcninfo.txtime += BCN_INTV_osticks - LMIC.drift;
        LMIC.bcninfo.time   += BCN_INTV_sec;
        LMIC.missedBcns++;
        // Delay any possible TX after surmised beacon - it's there although we missed it
        txDelay(LMIC.bcninfo.txtime + BCN_RESERVE_osticks, 4);
        // if too many missed beacons or we lose sync, drop back to Class A.
        if( LMIC.missedBcns > MAX_MISSED_BCNS ||
            LMIC.bcnRxsyms > MAX_RXSYMS ) {
            LMIC.opmode &= ~(OP_TRACK|OP_PINGABLE|OP_PINGINI|OP_REJOIN);
            reportEventAndUpdate(EV_LOST_TSYNC);
            return;
        }
    }
    LMIC.bcnRxtime = LMIC.bcninfo.txtime + BCN_INTV_osticks - calcRxWindow(0,DR_BCN);
    LMIC.bcnRxsyms = LMIC.rxsyms;
  rev:
    LMICbandplan_advanceBeaconChannel();
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & OP_PINGINI) != 0 )
        rxschedInit(&LMIC.ping);  // note: reuses LMIC.frame buffer!
#endif // !DISABLE_PING
    reportEventAndUpdate(ev);
}

// job entry: time to start receiving a beacon.
static void startRxBcn (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    LMIC.osjob.func = FUNC_ADDR(processBeacon);
    radioRx();
}
#endif // !DISABLE_BEACONS


#if !defined(DISABLE_PING)
// job entry: time to start receiving in our scheduled downlink slot.
static void startRxPing (xref2osjob_t osjob) {
    LMIC_API_PARAMETER(osjob);

    LMIC.osjob.func = FUNC_ADDR(processPingRx);
    radioRx();
}
#endif // !DISABLE_PING


// Decide what to do next for the MAC layer of a device. Inner part.
// Only called from outer part.
static void engineUpdate_inner (void) {
#if LMIC_DEBUG_LEVEL > 0
    LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": engineUpdate, opmode=0x%x\n", os_getTime(), LMIC.opmode);
#endif
    // Check for ongoing state: scan or TX/RX transaction
    if( (LMIC.opmode & (OP_SCAN|OP_TXRXPEND|OP_SHUTDOWN)) != 0 )
        return;

#if !defined(DISABLE_JOIN)
    if( LMIC.devaddr == 0 && (LMIC.opmode & OP_JOINING) == 0 ) {
        LMIC_startJoining();
        return;
    }
    // we're joined but LinkTracking says we're out of luck...
    if ( LMIC.devaddr != 0 && (LMIC.opmode & OP_UNJOIN) != 0 ) {
        LMIC.opmode &= ~OP_UNJOIN;
        LMIC_unjoinAndRejoin();
        return;
    }
#endif // !DISABLE_JOIN

    ostime_t now    = os_getTime();
    ostime_t txbeg  = 0;

#if !defined(DISABLE_BEACONS)
    ostime_t rxtime = 0;

    if( (LMIC.opmode & OP_TRACK) != 0 ) {
        // We are tracking a beacon
        // formerly asserted ( now - (LMIC.bcnRxtime - os_getRadioRxRampup()) <= 0 );
        rxtime = LMIC.bcnRxtime - os_getRadioRxRampup();
        if (now - rxtime < 0) {
            // too late: drop out of Class B.
            LMIC.opmode &= ~(OP_TRACK|OP_PINGABLE|OP_PINGINI|OP_REJOIN);
            reportEventNoUpdate(EV_LOST_TSYNC);
            return;
        }
    }
#endif // !DISABLE_BEACONS

    if( (LMIC.opmode & (OP_JOINING|OP_REJOIN|OP_TXDATA|OP_POLL)) != 0 ) {
        // Assuming txChnl points to channel which first becomes available again.
        bit_t jacc = ((LMIC.opmode & (OP_JOINING|OP_REJOIN)) != 0 ? 1 : 0);
        // Find next suitable channel and return availability time
        if( (LMIC.opmode & OP_NEXTCHNL) != 0 ) {
            txbeg = LMIC.txend = LMICbandplan_nextTx(now);
            LMIC.opmode &= ~OP_NEXTCHNL;
        } else {
            // no need to consider anything but LMIC.txend.
            txbeg = LMIC.txend;
        }
        // Delayed TX or waiting for duty cycle?
        if( (LMIC.globalDutyRate != 0 || (LMIC.opmode & OP_RNDTX) != 0)  &&  (txbeg - LMIC.globalDutyAvail) < 0 )
            txbeg = LMIC.globalDutyAvail;
#if !defined(DISABLE_BEACONS)
        // If we're tracking a beacon...
        // then make sure TX-RX transaction is complete before beacon
        if( (LMIC.opmode & OP_TRACK) != 0 &&
            txbeg + (jacc ? JOIN_GUARD_osticks : TXRX_GUARD_osticks) - rxtime > 0 ) {
            // Not enough time to complete TX-RX before beacon - postpone after beacon.
            // In order to avoid clustering of postponed TX right after beacon randomize start!
            txDelay(rxtime + BCN_RESERVE_osticks, 16);
            txbeg = 0;
            goto checkrx;
        }
#endif // !DISABLE_BEACONS
        // Earliest possible time vs overhead to setup radio
        if( txbeg - (now + TX_RAMPUP) < 0 ) {
            // We could send right now!
            txbeg = now;
            dr_t txdr = (dr_t)LMIC.datarate;
#if !defined(DISABLE_JOIN)
            if( jacc ) {
                u1_t ftype;
                if( (LMIC.opmode & OP_REJOIN) != 0 ) {
#if CFG_region != LMIC_REGION_as923
                    // in AS923 v1.1 or older, no need to change the datarate.
                    // otherwise we need to check feasibility.
                    txdr = lowerDR(txdr, LMIC.rejoinCnt);
#endif
                }
                ftype = HDR_FTYPE_JREQ;
                buildJoinRequest(ftype);
                LMIC.osjob.func = FUNC_ADDR(jreqDone);
            } else
#endif // !DISABLE_JOIN
            {
                if( LMIC.seqnoDn >= 0xFFFFFF80 ) {
                    // Imminent roll over - proactively reset MAC
                    EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_ROLL_OVER,
                                        e_.eui    = MAIN::CDEV->getEui(),
                                        e_.info   = LMIC.seqnoDn,
                                        e_.info2  = 0));
                    // Device has to react! NWK will not roll over and just stop sending.
                    // Thus, we have N frames to detect a possible lock up.
                  reset:
                    os_setCallback(&LMIC.osjob, FUNC_ADDR(runReset));
                    return;
                }
                if( (LMIC.txCnt==0 && LMIC.seqnoUp == 0xFFFFFFFF) ) {
                    // Roll over of up seq counter
                    EV(specCond, ERR, (e_.reason = EV::specCond_t::UPSEQNO_ROLL_OVER,
                                       e_.eui    = MAIN::CDEV->getEui(),
                                       e_.info2  = LMIC.seqnoUp));
                    // Do not run RESET event callback from here!
                    // App code might do some stuff after send unaware of RESET.
                    goto reset;
                }
                if (! buildDataFrame()) {
                    // can't transmit this message. Report completion.
                    initTxrxFlags(__func__, TXRX_LENERR);
                    if (LMIC.pendTxConf || LMIC.txCnt) {
                        orTxrxFlags(__func__, TXRX_NACK);
                    }
                    LMIC.opmode &= ~(OP_POLL|OP_RNDTX|OP_TXDATA|OP_TXRXPEND);
                    LMIC.dataBeg = LMIC.dataLen = 0;
                    reportEventNoUpdate(EV_TXCOMPLETE);
                    return;
                }
                LMIC.osjob.func = FUNC_ADDR(updataDone);
            } // end of else (not joining)
            LMIC.rps    = setCr(updr2rps(txdr), (cr_t)LMIC.errcr);
            LMIC.dndr   = txdr;  // carry TX datarate (can be != LMIC.datarate) over to txDone/setupRx1
            LMIC.opmode = (LMIC.opmode & ~(OP_POLL|OP_RNDTX)) | OP_TXRXPEND | OP_NEXTCHNL;
            LMICbandplan_updateTx(txbeg);
            // limit power to value asked in adr
            LMIC.radio_txpow = LMIC.txpow > LMIC.adrTxPow ? LMIC.adrTxPow : LMIC.txpow;
            reportEventNoUpdate(EV_TXSTART);
            os_radio(RADIO_TX);
            return;
        }
        // Cannot yet TX
        if( (LMIC.opmode & OP_TRACK) == 0 )
            goto txdelay; // We don't track the beacon - nothing else to do - so wait for the time to TX
        // Consider RX tasks
        if( txbeg == 0 ) // zero indicates no TX pending
            txbeg += 1;  // TX delayed by one tick (insignificant amount of time)
    } else {
        // No TX pending - no scheduled RX
        if( (LMIC.opmode & OP_TRACK) == 0 )
            return;
    }

#if !defined(DISABLE_BEACONS)
    // Are we pingable?
  checkrx:
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & OP_PINGINI) != 0 ) {
        // One more RX slot in this beacon period?
        if( rxschedNext(&LMIC.ping, now+os_getRadioRxRampup()) ) {
            if( txbeg != 0  &&  (txbeg - LMIC.ping.rxtime) < 0 )
                goto txdelay;
            LMIC.rxsyms  = LMIC.ping.rxsyms;
            LMIC.rxtime  = LMIC.ping.rxtime;
            LMIC.freq    = LMIC.ping.freq;
            LMIC.rps     = dndr2rps(LMIC.ping.dr);
            LMIC.dataLen = 0;
            ostime_t rxtime_ping = LMIC.rxtime - os_getRadioRxRampup();
            // did we miss the time?
            if (now - rxtime_ping > 0) {
                LMIC.opmode &= ~(OP_TRACK|OP_PINGABLE|OP_PINGINI|OP_REJOIN);
                reportEventNoUpdate(EV_LOST_TSYNC);
            } else {
                os_setTimedCallback(&LMIC.osjob, rxtime_ping, FUNC_ADDR(startRxPing));
            }
            return;
        }
        // no - just wait for the beacon
    }
#endif // !DISABLE_PING

    if( txbeg != 0  &&  (txbeg - rxtime) < 0 )
        goto txdelay;

    LMICbandplan_setBcnRxParams();
    LMIC.rxsyms = LMIC.bcnRxsyms;
    LMIC.rxtime = LMIC.bcnRxtime;
    if( now - rxtime >= 0 ) {
        LMIC.osjob.func = FUNC_ADDR(processBeacon);

        radioRx();
        return;
    }
    os_setTimedCallback(&LMIC.osjob, rxtime, FUNC_ADDR(startRxBcn));
    return;
#endif // !DISABLE_BEACONS

  txdelay:
    EV(devCond, INFO, (e_.reason = EV::devCond_t::TX_DELAY,
                       e_.eui    = MAIN::CDEV->getEui(),
                       e_.info   = osticks2ms(txbeg-now),
                       e_.info2  = LMIC.seqnoUp-1));
    LMIC_X_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": next engine update in %"LMIC_PRId_ostime_t"\n", now, txbeg-TX_RAMPUP);
    os_setTimedCallback(&LMIC.osjob, txbeg-TX_RAMPUP, FUNC_ADDR(runEngineUpdate));
}

// Decide what to do next for the MAC layer of a device.
// Outer part. Safe to call from anywhere; defers if it
// detects a recursive call.
static void engineUpdate (void) {
    lmic_engine_update_state_t state;

    state = LMIC.engineUpdateState;
    if (state == lmic_EngineUpdateState_idle) {
        LMIC.engineUpdateState = lmic_EngineUpdateState_busy;
        do  {
            engineUpdate_inner();
            state = LMIC.engineUpdateState - 1;
            LMIC.engineUpdateState = state;
            } while (state != lmic_EngineUpdateState_idle);
    } else {
        LMIC.engineUpdateState = lmic_EngineUpdateState_again;
    }
}

void LMIC_setAdrMode (bit_t enabled) {
    LMIC.adrEnabled = enabled ? FCT_ADREN : 0;
}


//  Should we have/need an ext. API like this?
void LMIC_setDrTxpow (dr_t dr, s1_t txpow) {
    setDrTxpow(DRCHG_SET, dr, txpow);
}


void LMIC_shutdown (void) {
    os_clearCallback(&LMIC.osjob);
    os_radio(RADIO_RST);
    LMIC.opmode |= OP_SHUTDOWN;
}

// reset the LMIC. This is called at startup; the clear of LMIC.osjob
// only works because the LMIC is guaranteed to be zero in that case.
// But it's also called at frame-count rollover; in that case we have
// to ensure that the user callback pointers are not clobbered.
void LMIC_reset (void) {
    EV(devCond, INFO, (e_.reason = EV::devCond_t::LMIC_EV,
                       e_.eui    = MAIN::CDEV->getEui(),
                       e_.info   = EV_RESET));
    os_radio(RADIO_RST);
    os_clearCallback(&LMIC.osjob);

    // save callback info, clear LMIC, restore.
    do {
        lmic_client_data_t  client = LMIC.client;

        os_clearMem((xref2u1_t)&LMIC,SIZEOFEXPR(LMIC));

        LMIC.client = client;
    } while (0);

    // LMIC.devaddr      =  0;      // true from os_clearMem().
    LMIC.devNonce     =  os_getRndU2();
    LMIC.opmode       =  OP_NONE;
    LMIC.errcr        =  CR_4_5;
    LMIC.adrEnabled   =  FCT_ADREN;
    resetJoinParams();
    LMIC.rxDelay      =  DELAY_DNW1;
    // LMIC.pendMacLen  =  0;
    // LMIC.pendMacPiggyback = 0;
    // LMIC.dn2Ans       = 0;
    // LMIC.macDlChannelAns = 0;
    // LMIC.macRxTimingSetupAns = 0;
#if !defined(DISABLE_PING)
    LMIC.ping.freq    =  FREQ_PING; // defaults for ping
    LMIC.ping.dr      =  DR_PING;   // ditto
    LMIC.ping.intvExp =  0xFF;
#endif // !DISABLE_PING

    LMICbandplan_resetDefaultChannels();
    DO_DEVDB(LMIC.devaddr,      devaddr);
    DO_DEVDB(LMIC.devNonce,     devNonce);
    DO_DEVDB(LMIC.dn2Dr,        dn2Dr);
    DO_DEVDB(LMIC.dn2Freq,      dn2Freq);
#if !defined(DISABLE_PING)
    DO_DEVDB(LMIC.ping.freq,    pingFreq);
    DO_DEVDB(LMIC.ping.dr,      pingDr);
    DO_DEVDB(LMIC.ping.intvExp, pingIntvExp);
#endif // !DISABLE_PING
#if LMIC_ENABLE_DeviceTimeReq
    LMIC.txDeviceTimeReqState = lmic_RequestTimeState_idle;
    LMIC.netDeviceTime = 0;     // the "invalid" time.
    LMIC.netDeviceTimeFrac = 0;
#endif // LMIC_ENABLE_DeviceTimeReq
}


void LMIC_init (void) {
    LMIC.opmode = OP_SHUTDOWN;
    LMICbandplan_init();
}


void LMIC_clrTxData (void) {
    u2_t opmode = LMIC.opmode;
    bit_t const txActive = opmode & OP_TXDATA;
    if (! txActive) {
        return;
    }
    LMIC.pendTxLen = 0;
    opmode &= ~(OP_TXDATA | OP_POLL);
    if (! (opmode & OP_JOINING)) {
        // in this case, we are joining, and the TX data
        // is just pending.
        opmode &= ~(OP_TXRXPEND);
    }

    LMIC.opmode = opmode;

    if (txActive)
        reportEventNoUpdate(EV_TXCANCELED);

    if( (LMIC.opmode & (OP_JOINING|OP_SCAN)) != 0 ) // do not interfere with JOINING
        return;
    os_clearCallback(&LMIC.osjob);
    os_radio(RADIO_RST);
    engineUpdate();
}

dr_t LMIC_feasibleDataRateForFrame(dr_t dr, u1_t payloadSize) {
    if (payloadSize > MAX_LEN_PAYLOAD) {
        return dr;
    }

    const u1_t frameSize = payloadSize + OFF_DAT_OPTS + 5;
    dr_t trialDr, nextDr;

    for (trialDr = dr; ;) {
        if (! LMICbandplan_isDataRateFeasible(trialDr))
            break;
        u1_t maxSizeThisDr = LMICbandplan_maxFrameLen(trialDr);
        if (maxSizeThisDr == 0) {
            break;
        } else if (frameSize <= maxSizeThisDr) {
            // we found one that is feasible!
            return trialDr;
        }
        // try the next DR
        nextDr = incDR(trialDr);
        if (nextDr == trialDr)
            break;
        trialDr = nextDr;
    }

    // if we get here, we didn't find a working dr.
    return dr;
}

static bit_t isTxPathBusy(void) {
    return (LMIC.opmode & (OP_TXDATA|OP_JOINING)) != 0;
}

static bit_t adjustDrForFrameIfNotBusy(u1_t len) {
    if (isTxPathBusy()) {
        return 0;
    }
    dr_t newDr = LMIC_feasibleDataRateForFrame(LMIC.datarate, len);
    if (newDr != LMIC.datarate) {
        setDrTxpow(DRCHG_FRAMESIZE, newDr, KEEP_TXPOW);
    }
    return 1;
}

void LMIC_setTxData (void) {
    adjustDrForFrameIfNotBusy(LMIC.pendTxLen);
    LMIC_setTxData_strict();
}

void LMIC_setTxData_strict (void) {
    LMICOS_logEventUint32(__func__, ((u4_t)LMIC.pendTxPort << 24u) | ((u4_t)LMIC.pendTxConf << 16u) | (LMIC.pendTxLen << 0u));
    LMIC.opmode |= OP_TXDATA;
    if( (LMIC.opmode & OP_JOINING) == 0 ) {
        LMIC.txCnt = 0;             // reset the confirmed uplink FSM
        LMIC.upRepeatCount = 0;     // reset the unconfirmed repeat FSM
    }
    engineUpdate();
}


// send a message, attempting to adjust TX data rate
lmic_tx_error_t LMIC_setTxData2 (u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed) {
    adjustDrForFrameIfNotBusy(dlen);
    return LMIC_setTxData2_strict(port, data, dlen, confirmed);
}

// send a message w/o callback; do not adjust data rate
lmic_tx_error_t LMIC_setTxData2_strict (u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed) {
    if ( LMIC.opmode & OP_TXDATA ) {
        // already have a message queued
        return LMIC_ERROR_TX_BUSY;
    }
    if( dlen > SIZEOFEXPR(LMIC.pendTxData) )
        return LMIC_ERROR_TX_TOO_LARGE;
    if( data != (xref2u1_t)0 )
        os_copyMem(LMIC.pendTxData, data, dlen);
    LMIC.pendTxConf = confirmed;
    LMIC.pendTxPort = port;
    LMIC.pendTxLen  = dlen;
    LMIC_setTxData_strict();
    if ( (LMIC.opmode & OP_TXDATA) == 0 ) {
        if (LMIC.txrxFlags & TXRX_LENERR) {
            return LMIC_ERROR_TX_NOT_FEASIBLE;
        } else {
            // data has already been completed with error for some reason
            return LMIC_ERROR_TX_FAILED;
        }
    }
    return 0;
}

// send a message with callback; try to adjust data rate
lmic_tx_error_t LMIC_sendWithCallback (
    u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed,
    lmic_txmessage_cb_t *pCb, void *pUserData
) {
    adjustDrForFrameIfNotBusy(dlen);
    return LMIC_sendWithCallback_strict(port, data, dlen, confirmed, pCb, pUserData);
}

// send a message with callback; do not adjust datarate
lmic_tx_error_t LMIC_sendWithCallback_strict (
    u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed,
    lmic_txmessage_cb_t *pCb, void *pUserData
) {
    lmic_tx_error_t const result = LMIC_setTxData2_strict(port, data, dlen, confirmed);
    if (result == 0) {
        LMIC.client.txMessageCb = pCb;
        LMIC.client.txMessageUserData = pUserData;
    }
    return result;
}


// Send a payload-less message to signal device is alive
void LMIC_sendAlive (void) {
    LMIC.opmode |= OP_POLL;
    engineUpdate();
}


// Check if other networks are around.
void LMIC_tryRejoin (void) {
    LMIC.opmode |= OP_REJOIN;
    engineUpdate();
}

//! \brief Setup given session keys
//! and put the MAC in a state as if
//! a join request/accept would have negotiated just these keys.
//! It is crucial that the combinations `devaddr/nwkkey` and `devaddr/artkey`
//! are unique within the network identified by `netid`.
//! NOTE: on Harvard architectures when session keys are in flash:
//!  Caller has to fill in LMIC.{nwk,art}Key  before and pass {nwk,art}Key are NULL
//! \param netid a 24 bit number describing the network id this device is using
//! \param devaddr the 32 bit session address of the device. It is strongly recommended
//!    to ensure that different devices use different numbers with high probability.
//! \param nwkKey  the 16 byte network session key used for message integrity.
//!     If NULL the caller has copied the key into `LMIC.nwkKey` before.
//! \param artKey  the 16 byte application router session key used for message confidentiality.
//!     If NULL the caller has copied the key into `LMIC.artKey` before.

// TODO(tmm@mcci.com) we ought to also save the channels that were returned by the
// join accept; right now this has to be done by the caller (or it doesn't get done).
void LMIC_setSession (u4_t netid, devaddr_t devaddr, xref2u1_t nwkKey, xref2u1_t artKey) {
    LMIC.netid = netid;
    LMIC.devaddr = devaddr;
    if( nwkKey != (xref2u1_t)0 )
        os_copyMem(LMIC.nwkKey, nwkKey, 16);
    if( artKey != (xref2u1_t)0 )
        os_copyMem(LMIC.artKey, artKey, 16);

    LMICbandplan_setSessionInitDefaultChannels();

    LMIC.opmode &= ~(OP_JOINING|OP_TRACK|OP_UNJOIN|OP_REJOIN|OP_TXRXPEND|OP_PINGINI);
    LMIC.opmode |= OP_NEXTCHNL;
    stateJustJoined();
    // transition to the ADR_ACK_DELAY state.
    setAdrAckCount(LINK_CHECK_CONT);

    DO_DEVDB(LMIC.netid,   netid);
    DO_DEVDB(LMIC.devaddr, devaddr);
    DO_DEVDB(LMIC.nwkKey,  nwkkey);
    DO_DEVDB(LMIC.artKey,  artkey);
    DO_DEVDB(LMIC.seqnoUp, seqnoUp);
    DO_DEVDB(LMIC.seqnoDn, seqnoDn);
}

// Enable/disable link check validation.
// LMIC sets the ADRACKREQ bit in UP frames if there were no DN frames
// for a while. It expects the network to provide a DN message to prove
// connectivity with a span of UP frames. If this no such prove is coming
// then the datarate is lowered and a LINK_DEAD event is generated.
// This mode can be disabled and no connectivity prove (ADRACKREQ) is requested
// nor is the datarate changed.
// This must be called only if a session is established (e.g. after EV_JOINED)
void LMIC_setLinkCheckMode (bit_t enabled) {
    LMIC.adrChanged = 0;
    LMIC.adrAckReq = enabled ? LINK_CHECK_INIT : LINK_CHECK_OFF;
}

// Sets the max clock error to compensate for (defaults to 0, which
// allows for +/- 640 at SF7BW250). MAX_CLOCK_ERROR represents +/-100%,
// so e.g. for a +/-1% error you would pass MAX_CLOCK_ERROR * 1 / 100.
void LMIC_setClockError(u2_t error) {
    LMIC.client.clockError = error;
}

// \brief return the uplink sequence number for the next transmission.
// This simple getter returns the uplink sequence number maintained by the LMIC engine.
// The caller should store the value and restore it (see LMIC_setSeqnoUp) on
// LMIC initialization to ensure monotonically increasing sequence numbers.
// It's also useful in debugging, as it allows you to correlate a debug trace event with
// a specific packet sent over the air.
u4_t LMIC_getSeqnoUp(void) {
    return LMIC.seqnoUp;
}

// \brief set the uplink sequence number for the next transmission.
// Use the function on startup to ensure that the next transmission uses
// a sequence number higher than the last transmission.
u4_t LMIC_setSeqnoUp(u4_t seq_no) {
    u4_t last = LMIC.seqnoUp;
    LMIC.seqnoUp = seq_no;
    return last;
}

// \brief return the current session keys returned from join.
void LMIC_getSessionKeys (u4_t *netid, devaddr_t *devaddr, xref2u1_t nwkKey, xref2u1_t artKey) {
    *netid = LMIC.netid;
    *devaddr = LMIC.devaddr;
    memcpy(artKey, LMIC.artKey, sizeof(LMIC.artKey));
    memcpy(nwkKey, LMIC.nwkKey, sizeof(LMIC.nwkKey));
}

// \brief post an asynchronous request for the network time.
void LMIC_requestNetworkTime(lmic_request_network_time_cb_t *pCallbackfn, void *pUserData) {
#if LMIC_ENABLE_DeviceTimeReq
    if (LMIC.txDeviceTimeReqState == lmic_RequestTimeState_idle) {
        LMIC.txDeviceTimeReqState = lmic_RequestTimeState_tx;
        LMIC.client.pNetworkTimeCb = pCallbackfn;
        LMIC.client.pNetworkTimeUserData = pUserData;
        return;
    }
#endif // LMIC_ENABLE_DeviceTimeReq
    // if no device time support, or if not in proper state,
    // report a failure.
    if (pCallbackfn != NULL)
        (*pCallbackfn)(pUserData, /* false */ 0);
}

// \brief return local/remote time pair (if valid, and DeviceTimeReq enabled),
// return true for success, false for error. We adjust the sampled OS time
// back in time to the nearest second boundary.
int LMIC_getNetworkTimeReference(lmic_time_reference_t *pReference) {
#if LMIC_ENABLE_DeviceTimeReq
    if (pReference != NULL &&       // valid parameter, and
        LMIC.netDeviceTime != 0) {  // ... we have a reasonable answer.
        const ostime_t tAdjust = LMIC.netDeviceTimeFrac * ms2osticks(1000) / 256;

        pReference->tLocal = LMIC.localDeviceTime - tAdjust;
        pReference->tNetwork = LMIC.netDeviceTime;
        return 1;
    }
#else
    LMIC_API_PARAMETER(pReference);
#endif // LMIC_ENABLE_DeviceTimeReq
    return 0;
}
