/*

Module: compliance-otaa-halconfig.ino

Function:
    Test program for developing and checking LMIC compliance test support.

Copyright and License:
    Please see accompanying LICENSE file.

Author:
    Terry Moore, MCCI Corporation   March 2019

*/

#include <Arduino.h>

#include <arduino_lmic.h>
#include <arduino_lmic_hal_boards.h>
#include <arduino_lmic_lorawan_compliance.h>

#include <SPI.h>
class cEventQueue;

#define APPLICATION_VERSION ARDUINO_LMIC_VERSION_CALC(3,0,99,10)

//
// For compliance tests with the RWC5020A, we use the default addresses
// from the tester; except that we use APPKEY 0,..., 0, 2, to avoid
// collisions with a registered app on TTN.
//

// This EUI must be in little-endian format, so least-significant-byte
// first.  This corresponds to 0x0000000000000001
// static const u1_t PROGMEM APPEUI[8]= { 1, 0, 0, 0, 0, 0, 0, 0 };
void os_getArtEui (u1_t* buf) { memset(buf, 0, 8); buf[0] = 1; }

// This should also be in little endian format, see above.
// This corresponds to 0x0000000000000001
// static const u1_t PROGMEM DEVEUI[8]= { 1, 0, 0, 0, 0, 0, 0, 0 };
void os_getDevEui (u1_t* buf) { memset(buf, 0, 8); buf[0] = 1; }

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply).
// static const u1_t PROGMEM APPKEY[16] = { 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 2 };
void os_getDevKey (u1_t* buf) { memset(buf, 0, 16); buf[15] = 2; }

// this data must be kept short -- max is 11 bytes for US DR0
static uint8_t mydata[] = { 0xCA, 0xFE, 0xF0, 0x0D };
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 5;

// global flag for test mode.
bool g_fTestMode = false;

// forward declarations
lmic_event_cb_t myEventCb;
lmic_rxmessage_cb_t myRxMessageCb;

const char * const evNames[] = { LMIC_EVENT_NAME_TABLE__INIT };

static void rtccount_begin();
static uint16_t rtccount_read();

#define NEED_USBD_LL_ConnectionState    0
#ifdef ARDUINO_ARCH_STM32
# ifdef _mcci_arduino_version
#  if _mcci_arduino_version < _mcci_arduino_version_calc(2, 5, 0, 10)
#   undef NEED_USBD_LL_ConnectionState
#   define NEED_USBD_LL_ConnectionState 1
#  endif // _mcci_arduino_version < _mcci_arduino_version_calc(2, 5, 0, 10)
# endif // def _mcci_arduino_version
#endif // def ARDUINO_ARCH_STM32

#define NEED_STM32_ClockCalibration    0
#ifdef ARDUINO_ARCH_STM32
# ifdef _mcci_arduino_version
#  if _mcci_arduino_version <= _mcci_arduino_version_calc(2, 5, 0, 10)
#   undef NEED_STM32_ClockCalibration
#   define NEED_STM32_ClockCalibration 1
#  endif // _mcci_arduino_version <= _mcci_arduino_version_calc(2, 5, 0, 10)
# endif // def _mcci_arduino_version
# define SUPPORT_STM32_ClockCalibration 1
#else
# define SUPPORT_STM32_ClockCalibration 0
#endif // def ARDUINO_ARCH_STM32

/*

Name:	myEventCb()

Function:
        lmic_event_cb_t myEventCb;

        extern "C" { void myEventCb(void *pUserData, ev_t ev); }

Description:
        This function is registered for event notifications from the LMIC
        during setup() processing. Its main job is to display events in a
        user-friendly way.

Returns:
        No explicit result.

*/

static osjobcbfn_t eventjob_cb;

class cEventQueue {
public:
    cEventQueue() {};
    ~cEventQueue() {};

    struct eventnode_t {
        osjob_t     job;
        ev_t        event;
        const char *pMessage;
        uint32_t    datum;
        ostime_t    time;
        ostime_t    txend;
        ostime_t    rxtime;
        ostime_t    globalDutyAvail;
        u4_t        nLateRx;
        ostime_t    ticksLateRx;
        u4_t        freq;
        u2_t        rtccount;
        u2_t        opmode;
        u2_t        fcntDn;
        u2_t        fcntUp;
        rxsyms_t    rxsyms;
        rps_t       rps;
        u1_t        txChnl;
        u1_t        datarate;
        u1_t        txrxFlags;
        u1_t        saveIrqFlags;
    };

    bool getEvent(eventnode_t &node) {
        if (m_head == m_tail) {
            return false;
        }
        node = m_queue[m_head];
        if (++m_head == sizeof(m_queue) / sizeof(m_queue[0])) {
            m_head = 0;
        }
        return true;
    }

    bool putEvent(ev_t event, const char *pMessage = nullptr, uint32_t datum = 0) {
        auto i = m_tail + 1;
        if (i == sizeof(m_queue) / sizeof(m_queue[0])) {
            i = 0;
        }
        if (i != m_head) {
            auto const pn = &m_queue[m_tail];
            pn->job = LMIC.osjob;
            pn->time = os_getTime();
            pn->rtccount = rtccount_read();
            pn->txend = LMIC.txend;
            pn->rxtime = LMIC.rxtime;
            pn->globalDutyAvail = LMIC.globalDutyAvail;
            pn->event = event;
            pn->pMessage = pMessage;
            pn->datum = datum;
            pn->nLateRx = LMIC.radio.rxlate_count;
            pn->ticksLateRx = LMIC.radio.rxlate_ticks;
            pn->freq = LMIC.freq;
            pn->opmode = LMIC.opmode;
            pn->fcntDn = (u2_t) LMIC.seqnoDn;
            pn->fcntUp = (u2_t) LMIC.seqnoUp;
            pn->rxsyms = LMIC.rxsyms;
            pn->rps = LMIC.rps;
            pn->txChnl = LMIC.txChnl;
            pn->datarate = LMIC.datarate;
            pn->txrxFlags = LMIC.txrxFlags;
            pn->saveIrqFlags = LMIC.saveIrqFlags;
            m_tail = i;
            return true;
        } else {
            return false;
        }
    }

private:
    unsigned m_head, m_tail;
    eventnode_t m_queue[32];
    osjob_t m_job;
};

cEventQueue eventQueue;

#if LMIC_ENABLE_event_logging
extern "C" {
    void LMICOS_logEvent(const char *pMessage);
    void LMICOS_logEventUint32(const char *pMessage, uint32_t datum);
}

void LMICOS_logEvent(const char *pMessage)
    {
    eventQueue.putEvent(ev_t(-1), pMessage);
    }

void LMICOS_logEventUint32(const char *pMessage, uint32_t datum)
    {
    eventQueue.putEvent(ev_t(-2), pMessage, datum);
    }
#endif // LMIC_ENABLE_event_logging

hal_failure_handler_t log_assertion;

void log_assertion(const char *pMessage, uint16_t line) {
    eventQueue.putEvent(ev_t(-3), pMessage, line);
    eventPrintAll();
    Serial.println(F("***HALTED BY ASSERT***"));
    while (true)
        yield();
}

bool lastWasTxStart;
uint32_t lastTxStartTime;

void myEventCb(void *pUserData, ev_t ev) {
    eventQueue.putEvent(ev);

    if (ev == EV_TXSTART) {
        lastWasTxStart = true;
        lastTxStartTime = millis();
    } else if (ev == EV_RXSTART) {
        lastWasTxStart = false;
    }
    if (ev == EV_JOINING) {
        setupForNetwork(true);
    } else if (ev == EV_JOINED) {
        setupForNetwork(false);
    }
}

void eventPrint(cEventQueue::eventnode_t &e);
void printFcnts(cEventQueue::eventnode_t &e);
void printTxend(cEventQueue::eventnode_t &e);
void printRxtime(cEventQueue::eventnode_t &e);
void printLateStats(cEventQueue::eventnode_t &e);

void eventPrintAll(void) {
    while (eventPrintOne())
        ;
}

bool eventPrintOne(void) {
    cEventQueue::eventnode_t e;
    if (eventQueue.getEvent(e)) {
        eventPrint(e);
        return true;
    } else {
        return false;
    }
}

static void eventjob_cb(osjob_t *j) {
    eventPrintAll();
}

const char *getSfName(rps_t rps) {
    const char * const t[] = { "FSK", "SF7", "SF8", "SF9", "SF10", "SF11", "SF12", "SFrfu" };
    return t[getSf(rps)];
}

const char *getBwName(rps_t rps) {
    const char * const t[] = { "BW125", "BW250", "BW500", "BWrfu" };
    return t[getBw(rps)];
}

const char *getCrName(rps_t rps) {
    const char * const t[] = { "CR 4/5", "CR 4/6", "CR 4/7", "CR 4/8" };
    return t[getCr(rps)];
}

const char *getCrcName(rps_t rps) {
    return getNocrc(rps) ? "NoCrc" : "Crc";
}

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void printHex4(unsigned v) {
    printHex2(v >> 8u);
    printHex2(v);
}

void printSpace(void) {
    Serial.print(' ');
}

void printFreq(u4_t freq) {
    Serial.print(F(": freq="));
    Serial.print(freq / 1000000);
    Serial.print('.');
    Serial.print((freq % 1000000) / 100000);
}

void printRps(rps_t rps) {
    Serial.print(F(" rps=0x")); printHex2(rps);
    Serial.print(F(" (")); Serial.print(getSfName(rps));
    printSpace(); Serial.print(getBwName(rps));
    printSpace(); Serial.print(getCrName(rps));
    printSpace(); Serial.print(getCrcName(rps));
    Serial.print(F(" IH=")); Serial.print(unsigned(getIh(rps)));
    Serial.print(')');
}

void printOpmode(uint16_t opmode, char sep = ',') {
    if (sep != 0)
        Serial.print(sep);
    Serial.print(F(" opmode=")); Serial.print(opmode, HEX);
}

void printTxend(cEventQueue::eventnode_t &e) {
    Serial.print(F(", txend=")); Serial.print(e.txend);
    Serial.print(F(", avail=")); Serial.print(e.globalDutyAvail);
}

void printRxtime(cEventQueue::eventnode_t &e) {
    Serial.print(F(", rxtime=")); Serial.print(e.rxtime);
}

void printTxChnl(u1_t txChnl) {
    Serial.print(F(": ch="));
    Serial.print(unsigned(txChnl));
}

void printDatarate(u1_t datarate) {
    Serial.print(F(", datarate=")); Serial.print(unsigned(datarate));
}

void printTxrxflags(u1_t txrxFlags) {
    Serial.print(F(", txrxFlags=0x")); printHex2(txrxFlags);
    if (txrxFlags & TXRX_ACK)
        Serial.print(F("; Received ack"));
}

void printSaveIrqFlags(u1_t saveIrqFlags) {
    Serial.print(F(", saveIrqFlags 0x"));
    printHex2(saveIrqFlags);
}

void printLateStats(cEventQueue::eventnode_t &e) {
    Serial.print(F(", nLateRx="));
    Serial.print(e.nLateRx);
    Serial.print(F(" ticks="));
    Serial.print(e.ticksLateRx);
}

void printFcnts(cEventQueue::eventnode_t &e) {
    Serial.print(F(", FcntUp="));
    printHex4(e.fcntUp);
    Serial.print(F(", FcntDn="));
    printHex4(e.fcntDn);
}

#if LMIC_ENABLE_event_logging
// dump all the registers.
void printAllRegisters(void) {
    uint8_t regbuf[0x80];
    regbuf[0] = 0;
    hal_spi_read(1, regbuf + 1, sizeof(regbuf) - 1);

    for (unsigned i = 0; i < sizeof(regbuf); ++i) {
        if (i % 16 == 0) {
            printNl();
            printHex2(i);
        }
        Serial.print(((i % 8) == 0) ? F(" - ") : F(" "));
        printHex2(regbuf[i]);
    }

    // reset the radio, just in case the register dump caused issues.
    hal_pin_rst(0);
    delay(2);
    hal_pin_rst(2);
    delay(6);

    // restore the radio to idle.
    const uint8_t opmode = 0x88;    // LoRa and sleep.
    hal_spi_write(0x81, &opmode, 1);
}
#endif

void printNl(void) {
    Serial.println();
}

void eventPrint(cEventQueue::eventnode_t &e) {
    ev_t ev = e.event;

    Serial.print(e.time);
    Serial.print(F(" ("));
    Serial.print(osticks2ms(e.time));
#if SUPPORT_STM32_ClockCalibration
    Serial.print(F(" ms, lptim1="));
    Serial.print(e.rtccount);
    Serial.print(F("): "));
#else
    Serial.print(F(" ms): "));
#endif

    if (ev == ev_t(-1) || ev == ev_t(-2)) {
        Serial.print(e.pMessage);
        if (ev == ev_t(-2)) {
            Serial.print(F(", datum=0x")); Serial.print(e.datum, HEX);
        }
        printOpmode(e.opmode, '.');
    } else if (ev == ev_t(-3)) {
        Serial.print(e.pMessage);
        Serial.print(F(", line ")); Serial.print(e.datum);
        printFreq(e.freq);
        printTxend(e);
        printTxChnl(e.txChnl);
        printRps(e.rps);
        printOpmode(e.opmode);
        printTxrxflags(e.txrxFlags);
        printSaveIrqFlags(e.saveIrqFlags);
        printLateStats(e);
#if LMIC_ENABLE_event_logging
        printAllRegisters();
#endif
    } else {
        if (ev < sizeof(evNames) / sizeof(evNames[0])) {
            Serial.print(evNames[ev]);
        } else {
            Serial.print(F("Unknown event: "));
            Serial.print((unsigned) ev);
        }
        switch(ev) {
            case EV_SCAN_TIMEOUT:
                break;
            case EV_BEACON_FOUND:
                break;
            case EV_BEACON_MISSED:
                break;
            case EV_BEACON_TRACKED:
                break;
            case EV_JOINING:
                break;

            case EV_JOINED:
                printTxChnl(e.txChnl);
                printNl();
                do  {
                    u4_t netid = 0;
                    devaddr_t devaddr = 0;
                    u1_t nwkKey[16];
                    u1_t artKey[16];
                    LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
                    Serial.print(F("netid: "));
                    Serial.println(netid, DEC);
                    Serial.print(F("devaddr: "));
                    Serial.println(devaddr, HEX);
                    Serial.print(F("artKey: "));
                    for (size_t i=0; i<sizeof(artKey); ++i) {
                        if (i != 0)
                            Serial.print('-');
                        printHex2(artKey[i]);
                    }
                    printNl();
                    Serial.print(F("nwkKey: "));
                    for (size_t i=0; i<sizeof(nwkKey); ++i) {
                            if (i != 0)
                                    Serial.print('-');
                            printHex2(nwkKey[i]);
                    }
                } while (0);
                break;
            /*
            || This event is defined but not used in the code. No
            || point in wasting codespace on it.
            ||
            || case EV_RFU1:
            ||     Serial.println(F("EV_RFU1"));
            ||     break;
            */
            case EV_JOIN_FAILED:
                // print out rx info
                printFreq(e.freq);
                printRps(e.rps);
                printOpmode(e.opmode);
#if LMIC_ENABLE_event_logging
                printAllRegisters();
#endif
                break;

            case EV_REJOIN_FAILED:
                // this event means that someone tried a rejoin, and it failed.
                // it doesn't really mean anything bad, it's just advisory.
                break;

            case EV_TXCOMPLETE:
                printTxChnl(e.txChnl);
                printRps(e.rps);
                printTxrxflags(e.txrxFlags);
                printFcnts(e);
                printTxend(e);
                printLateStats(e);
                break;
            case EV_LOST_TSYNC:
                break;
            case EV_RESET:
                break;
            case EV_RXCOMPLETE:
                break;
            case EV_LINK_DEAD:
                break;
            case EV_LINK_ALIVE:
                break;
            /*
            || This event is defined but not used in the code. No
            || point in wasting codespace on it.
            ||
            || case EV_SCAN_FOUND:
            ||    Serial.println(F("EV_SCAN_FOUND"));
            ||    break;
            */
            case EV_TXSTART:
                // this event tells us that a transmit is about to start.
                // but printing here is bad for timing.
                printTxChnl(e.txChnl);
                printRps(e.rps);
                printDatarate(e.datarate);
                printOpmode(e.opmode);
                printTxend(e);
                break;

            case EV_RXSTART:
                printFreq(e.freq);
                printRps(e.rps);
                printDatarate(e.datarate);
                printOpmode(e.opmode);
                printTxend(e);
                printRxtime(e);
                Serial.print(F(", rxsyms=")); Serial.print(unsigned(e.rxsyms));
                break;

            case EV_JOIN_TXCOMPLETE:
                printSaveIrqFlags(e.saveIrqFlags);
                printLateStats(e);
                break;

            default:
                break;
        }
    }
    printNl();
}

/*

Name:   myRxMessageCb()

Function:
        Handle received LoRaWAN downlink messages.

Definition:
        lmic_rxmessage_cb_t myRxMessageCb;

        extern "C" {
            void myRxMessageCb(
                void *pUserData,
                uint8_t port,
                const uint8_t *pMessage,
                size_t nMessage
                );
        }

Description:
        This function is called whenever a non-Join downlink message
        is received over LoRaWAN by LMIC. Its job is to invoke the
        compliance handler (if compliance support is needed), and
        then decode any non-compliance messages.

Returns:
        No explicit result.

*/

void myRxMessageCb(
    void *pUserData,
    uint8_t port,
    const uint8_t *pMessage,
    size_t nMessage
) {
    lmic_compliance_rx_action_t const action = LMIC_complianceRxMessage(port, pMessage, nMessage);
    switch (action) {
        case LMIC_COMPLIANCE_RX_ACTION_START: {
            Serial.println(F("Enter test mode"));
            os_clearCallback(&sendjob);
            g_fTestMode = true;
            return;
        }
        case LMIC_COMPLIANCE_RX_ACTION_END: {
            Serial.println(F("Exit test mode"));
            g_fTestMode = false;
            // we're in the LMIC, we don't want to send from here. Schedule a job.
            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
            return;
        }
        case LMIC_COMPLIANCE_RX_ACTION_IGNORE: {
            if (port == LORAWAN_PORT_COMPLIANCE) {
                Serial.print(F("Received test packet 0x"));
                if (nMessage > 0)
                    printHex2(pMessage[0]);
                Serial.print(F(" length "));
                Serial.println((unsigned) nMessage);
            }
            return;
        }
        default:
            // continue.
            break;
    }

    Serial.print(F("Received message on port "));
    Serial.print(port);
    Serial.print(F(": "));
    Serial.print(unsigned(nMessage));
    Serial.println(F(" bytes"));
    }

lmic_txmessage_cb_t sendComplete;

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        sendComplete(j, 0);
    } else if (g_fTestMode) {
        Serial.println(F("test mode, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        if (LMIC_sendWithCallback_strict(1, mydata, sizeof(mydata), 0, sendComplete, j) == 0) {
            Serial.println(F("Packet queued"));
        } else {
            Serial.println(F("Packet queue failure; sleeping"));
            sendComplete(j, 0);
        }
    }
}

void sendComplete(
        void *pUserData,
        int fSuccess
) {
    osjob_t * const j = (osjob_t *) pUserData;

    if (! fSuccess)
        Serial.println(F("sendComplete: uplink failed"));

    if (! g_fTestMode) {
            // Schedule next transmission
            os_setTimedCallback(j, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}

void myFail(const char *pMessage) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        // alert
        Serial.println(pMessage);
        // flash lights, sleep.
        for (int i = 0; i < 5; ++i) {
            digitalWrite(LED_BUILTIN, 1);
            delay(100);
            digitalWrite(LED_BUILTIN, 0);
            delay(900);
        }
    }
}

void setup() {
    delay(5000);
    while (! Serial)
        ;
    Serial.begin(115200);
    setup_printSignOn();
    setup_calibrateSystemClock();

    // LMIC init using the computed target
    const auto pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    // don't die mysteriously; die noisily.
    if (pPinMap == nullptr) {
        myFail("board not known to library; add pinmap or update getconfig_thisboard.cpp");
    }

    // now that we have a pinmap, initalize the low levels accordingly.
    hal_set_failure_handler(log_assertion);
    os_init_ex(pPinMap);

    // LMIC_reset() doesn't affect callbacks, so we can do this first.
    if (! (LMIC_registerRxMessageCb(myRxMessageCb, /* userData */ nullptr) &&
           LMIC_registerEventCb(myEventCb, /* userData */ nullptr))) {
        myFail("couldn't register callbacks");
    }

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // do the network-specific setup prior to join.
    setupForNetwork(false);

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void setup_printSignOnDashLine(void)
    {
    for (unsigned i = 0; i < 78; ++i)
        Serial.print('-');

    printNl();
    }

static constexpr const char *filebasename2(const char *s, const char *p) {
    return p[0] == '\0'                     ? s                             :
           (p[0] == '/' || p[0] == '\\')    ? filebasename2(p + 1, p + 1)   :
                                              filebasename2(s, p + 1)       ;
}

static constexpr const char *filebasename(const char *s)
    {
    return filebasename2(s, s);
    }

void printVersionFragment(char sep, uint8_t v) {
    if (sep != 0) {
        Serial.print(sep);
    }
    Serial.print(unsigned(v));
}

void printVersion(uint32_t v) {
    printVersionFragment(0, uint8_t(v >> 24u));
    printVersionFragment('.', uint8_t(v >> 16u));
    printVersionFragment('.', uint8_t(v >> 8u));
    if (uint8_t(v) != 0) {
        printVersionFragment('.', uint8_t(v));
    }
}

void setup_printSignOn()
    {
    printNl();

    setup_printSignOnDashLine();

    Serial.println(filebasename(__FILE__));
    Serial.print(F("Version "));
    printVersion(APPLICATION_VERSION);
    Serial.print(F("\nLMIC version "));
    printVersion(ARDUINO_LMIC_VERSION);
    Serial.print(F(" configured for region "));
    Serial.print(CFG_region);
    Serial.println(F(".\nRemember to select 'Line Ending: Newline' at the bottom of the monitor window."));

    setup_printSignOnDashLine();
    printNl();
    }

void setupForNetwork(bool preJoin) {
#if CFG_LMIC_US_like
    LMIC_selectSubBand(0);
#endif
}

void loop() {
    os_runloop_once();

    if (lastWasTxStart && millis() - lastTxStartTime > 10000) {
        /* ugh. TX timed out */
        Serial.println(F("Tx timed out"));
#if LMIC_ENABLE_event_logging
        printAllRegisters();
#endif
        LMIC_clrTxData();
        lastWasTxStart = false;
    }

    if ((LMIC.opmode & OP_TXRXPEND) == 0 &&
        !os_queryTimeCriticalJobs(ms2osticks(1000))) {
           eventPrintAll();
    }
}


// there's a problem with running 2.5 of the MCCI STM32 BSPs;
// hack around it.
#if NEED_USBD_LL_ConnectionState
uint32_t USBD_LL_ConnectionState(void) {
  return 1;
}
#endif // NEED_USBD_LL_ConnectionState

static constexpr bool kMustCalibrateLSE = NEED_STM32_ClockCalibration;          // _mcci_arduino_version indicates that LSE clock is used.
static constexpr bool kCanCalibrateLSE = SUPPORT_STM32_ClockCalibration;

void setup_calibrateSystemClock(void) {
    if (kMustCalibrateLSE) {
        Serial.println("need to calibrate clock");
#if NEED_STM32_ClockCalibration
        Stm32_CalibrateSystemClock();
#endif // NEED_STM32_ClockCalibration
        Serial.println("setting LPTIM1");
        // set clock rate error to 0.4%
        LMIC_setClockError(4 * MAX_CLOCK_ERROR / 1000);
        rtccount_begin();
    } else if (kCanCalibrateLSE) {
        Serial.println("assuming BIOS has calibrated clock, setting LPTIM1");
        LMIC_setClockError(4 * MAX_CLOCK_ERROR / 1000);
        rtccount_begin();
    } else {
        Serial.println("calibration not supported");
    }
}

#if NEED_STM32_ClockCalibration

// RTC needs to be initialized before we calibrate the clock.
bool rtcbegin() {
    RTC_TimeTypeDef	Time;
    RTC_DateTypeDef	Date;
    uint32_t RtcClock;
    RTC_HandleTypeDef	hRtc;

    memset(&hRtc, 0, sizeof(hRtc));

    hRtc.Instance = RTC;
    hRtc.Init.HourFormat = RTC_HOURFORMAT_24;
    RtcClock = __HAL_RCC_GET_RTC_SOURCE();
    if (RtcClock == RCC_RTCCLKSOURCE_LSI)
        {
        hRtc.Init.AsynchPrediv = 37 - 1; /* 37kHz / 37 = 1000Hz */
        hRtc.Init.SynchPrediv = 1000 - 1; /* 1000Hz / 1000 = 1Hz */
        }
    else if (RtcClock == RCC_RTCCLKSOURCE_LSE)
        {
        hRtc.Init.AsynchPrediv = 128 - 1; /* 32768Hz / 128 = 256Hz */
        hRtc.Init.SynchPrediv = 256 - 1; /* 256Hz / 256 = 1Hz */
        }
    else
        {
        /*
        || use HSE clock --
        || we don't support use of HSE as RTC because it's connected to
        || TCXO_OUT, and that's controlled by the LoRaWAN software.
        */
        Serial.println(
            " HSE can not be used for RTC clock!"
            );
        return false;
        }


    hRtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hRtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hRtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hRtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&hRtc) != HAL_OK)
        {
        Serial.println(
            "HAL_RTC_Init() failed"
            );
        return false;
        }

    /* Initialize RTC and set the Time and Date */
    if (HAL_RTCEx_BKUPRead(&hRtc, RTC_BKP_DR0) != 0x32F2)
        {
        Time.Hours = 0x0;
        Time.Minutes = 0x0;
        Time.Seconds = 0x0;
        Time.SubSeconds = 0x0;
        Time.TimeFormat = RTC_HOURFORMAT12_AM;
        Time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        Time.StoreOperation = RTC_STOREOPERATION_RESET;

        if (HAL_RTC_SetTime(
            &hRtc,
            &Time,
            RTC_FORMAT_BIN
            ) != HAL_OK)
            {
            Serial.print(
                "HAL_RTC_SetTime() failed"
                );
            return false;
            }

        /* Sunday 1st January 2017 */
        Date.WeekDay = RTC_WEEKDAY_SUNDAY;
        Date.Month = RTC_MONTH_JANUARY;
        Date.Date = 0x1;
        Date.Year = 0x0;

        if (HAL_RTC_SetDate(
            &hRtc,
            &Date,
            RTC_FORMAT_BIN
            ) != HAL_OK)
            {
            Serial.print(
                "HAL_RTC_SetDate() failed"
                );
            return false;
            }

        HAL_RTCEx_BKUPWrite(&hRtc, RTC_BKP_DR0, 0x32F2);
        }

    /* Enable Direct Read of the calendar registers (not through Shadow) */
    HAL_RTCEx_EnableBypassShadow(&hRtc);

    HAL_RTC_DeactivateAlarm(&hRtc, RTC_ALARM_A);
    return true;
}

extern "C" {

static volatile uint32_t *gs_pAlarm;
static RTC_HandleTypeDef *gs_phRtc;

void RTC_IRQHandler(void)
    {
    HAL_RTC_AlarmIRQHandler(gs_phRtc);
    }

void HAL_RTC_AlarmAEventCallback(
    RTC_HandleTypeDef *	hRtc
    )
    {
    if (gs_pAlarm)
        *gs_pAlarm = 1;
    }

void HAL_RTC_MspInit(
    RTC_HandleTypeDef *	hRtc
    )
    {
    if (hRtc->Instance == RTC)
        {
        /* USER CODE BEGIN RTC_MspInit 0 */

        /* USER CODE END RTC_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_RTC_ENABLE();
        /* USER CODE BEGIN RTC_MspInit 1 */
        HAL_NVIC_SetPriority(RTC_IRQn, TICK_INT_PRIORITY, 0U);
        HAL_NVIC_EnableIRQ(RTC_IRQn);
        /* USER CODE END RTC_MspInit 1 */
        }
    }

void HAL_RTC_MspDeInit(
    RTC_HandleTypeDef *	hRtc
    )
    {
    if (hRtc->Instance == RTC)
        {
        /* USER CODE BEGIN RTC_MspDeInit 0 */
        HAL_NVIC_DisableIRQ(RTC_IRQn);
        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();
        /* USER CODE BEGIN RTC_MspDeInit 1 */

        /* USER CODE END RTC_MspDeInit 1 */
        }
    }

uint32_t HAL_AddTick(
    uint32_t delta
    )
    {
    extern __IO uint32_t uwTick;
    // copy old interrupt-enable state to flags.
    uint32_t const flags = __get_PRIMASK();

    // disable interrupts
    __set_PRIMASK(1);

    // observe uwTick, and advance it.
    uint32_t const tickCount = uwTick + delta;

    // save uwTick
    uwTick = tickCount;

    // restore interrupts (does nothing if ints were disabled on entry)
    __set_PRIMASK(flags);

    // return the new value of uwTick.
    return tickCount;
    }

} /* extern "C" */

uint32_t Stm32_CalibrateSystemClock(void)
    {
    uint32_t Calib;
    uint32_t CalibNew;
    uint32_t CalibLow;
    uint32_t CalibHigh;
    uint32_t mSecond;
    uint32_t mSecondNew;
    uint32_t mSecondLow;
    uint32_t mSecondHigh;
    bool fHaveSeenLow;
    bool fHaveSeenHigh;
    const bool fCalibrateMSI =  HAL_RCC_GetHCLKFreq() < 16000000;

    if (! rtcbegin()) {
        return 0;
    }

    if (fCalibrateMSI)
        {
        Calib = (RCC->ICSCR & RCC_ICSCR_MSITRIM) >> 24;
        }
    else
        {
        Calib = (RCC->ICSCR & RCC_ICSCR_HSITRIM) >> 8;
        }

    /* preapre to loop, setting suitable defaults */
    CalibNew = Calib;
    CalibLow = 0;
    CalibHigh = 0;
    mSecondLow = 0;
    mSecondHigh = 2000000;
    fHaveSeenLow = fHaveSeenHigh = false;

    /* loop until we have a new value */
    do	{
        /* meassure the # of millis per RTC second */
        mSecond = MeasureMicrosPerRtcSecond();

        /* invariant: */
        if (Calib == CalibNew)
            mSecondNew = mSecond;

        /* if mSecond is low, this meaans we must increase the system clock */
        if (mSecond <= 1000000)
            {
            Serial.print('-');
            /*
            || the following condition establishes that we're
            || below the target frequency, but closer than we've been
            || before (mSecondLow is the previous "low" limit). If
            || so, we reduce the limit, and capture the "low" calibration
            || value.
            */
            if (mSecond > mSecondLow)
                {
                mSecondLow = mSecond;
                CalibLow = Calib; /* save previous calibration value */
                fHaveSeenLow = true;
                }

            /*
            || if we are low, and we have never exceeded the high limit,
            || we can  increase the clock.
            */
            if (! fHaveSeenHigh)
                {
                if (fCalibrateMSI)
                    {
                    if (Calib < 0xFF)
                        {
                        ++Calib;
                        __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(Calib);
                        }
                    else
                        break;
                    }
                else
                    {
                    if (Calib < 0x1F)
                        {
                        ++Calib;
                        __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(Calib);
                        }
                    else
                        {
                        break;
                        }
                    }

                /* let the clock settle */
                delay(500);
                }
            }

        /* if mSecond is high, we must reduce the system clock */
        else
            {
            Serial.print('+');
            /*
            || the following condition establishes that we're
            || above the target frequency, but closer than we've been
            || before (mSecondHigh is the previous "high" limit). If
            || so, we reduce the limit, and capture the calibration
            || value.
            */
            if (mSecond < mSecondHigh)
                {
                mSecondHigh = mSecond;
                CalibHigh = Calib;
                fHaveSeenHigh = true;
                }

            /*
            || if we are above the target frequency, and we have
            || never raised the frequence, we can lower the
            || frequency
            */
            if (! fHaveSeenLow)
                {
                if (Calib == 0)
                    break;

                --Calib;
                if (fCalibrateMSI)
                    {
                    __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(Calib);
                    }
                else
                    {
                    __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(Calib);
                    }
                delay(500);
                }
            }
        } while ((Calib != CalibNew) &&
                (! fHaveSeenLow || !fHaveSeenHigh));

    //
    // We are going to take higher calibration value first and
    // it allows us not to call LMIC_setClockError().
    //
    if (fHaveSeenHigh)
        {
        mSecondNew = mSecondHigh;
        CalibNew = CalibHigh;
        }
    else if (fHaveSeenLow)
        {
        mSecondNew = mSecondLow;
        CalibNew = CalibLow;
        }
    else
        {
        // Use original value
        Serial.println(
            "?CalibrateSystemClock: can't calibrate"
            );
        }

    if (CalibNew != Calib)
        {
        Serial.print(CalibNew < Calib ? '+' : '-');
        if (fCalibrateMSI)
            {
            __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(CalibNew);
            }
        else
            {
            __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(CalibNew);
            }
        delay(500);
        }

    Serial.print(" 0x");
    Serial.println(CalibNew, HEX);
    return CalibNew;
    }

uint32_t
MeasureMicrosPerRtcSecond(
    void
    )
    {
    uint32_t second;
    uint32_t now;
    uint32_t start;
    uint32_t end;

    /* get the starting time */
    second = RTC->TR & (RTC_TR_ST | RTC_TR_SU);

    /* wait for a new second to start, and capture millis() in start */
    do	{
        now = RTC->TR & (RTC_TR_ST | RTC_TR_SU);
        start = micros();
        } while (second == now);

    /* update our second of interest */
    second = now;

    /* no point in watching the register until we get close */
    delay(500);

    /* wait for the next second to start, and capture millis() */
    do	{
        now = RTC->TR & (RTC_TR_ST | RTC_TR_SU);
        end = micros();
        } while (second == now);

    /* return the delta */
    return end - start;
    }
#endif // NEED_STM32_ClockCalibration

#if SUPPORT_STM32_ClockCalibration
static void rtccount_begin()
    {
    // enable clock to LPTIM1
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    auto const pLptim = LPTIM1;

    // set LPTIM1 clock to LSE clock.
    __HAL_RCC_LPTIM1_CONFIG(RCC_LPTIM1CLKSOURCE_LSE);

    // disable everything so we can tweak the CFGR
    pLptim->CR = 0;

    // disable interrupts (needs to be done while disabled globally)
    pLptim->IER = 0;

    // upcount from selected internal clock (which is LSE)
    auto rCfg = pLptim->CFGR & ~0x01FEEEDF;
    rCfg |=  0;
    pLptim->CFGR = rCfg;

    // enable the counter but don't start it
    pLptim->CR = LPTIM_CR_ENABLE;
    delayMicroseconds(100);

    // set ARR to max value so we can count from 0 to 0xFFFF.
    // must be done after enabling.
    pLptim->ARR = 0xFFFF;

    // start in continuous mode.
    pLptim->CR = LPTIM_CR_ENABLE | LPTIM_CR_CNTSTRT;
    }

static uint16_t rtccount_read()
    {
    auto const pLptim = LPTIM1;
    uint32_t v1, v2;

    for (v1 = pLptim->CNT & 0xFFFF; (v2 = pLptim->CNT & 0xFFFF) != v1; v1 = v2)
        /* loop */;

    return (uint16_t) v1;
    }

#else
static void rtccount_begin()
    {
    // nothing
    }

static uint16_t rtccount_read()
    {
    return 0;
    }
#endif
