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
        u4_t        freq;
        rps_t       rps;
        u2_t        opmode;
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
            pn->time = os_getTime();
            pn->txend = LMIC.txend;
            pn->event = event;
            pn->pMessage = pMessage;
            pn->datum = datum;
            pn->freq = LMIC.freq;
            pn->txChnl = LMIC.txChnl;
            pn->rps = LMIC.rps;
            pn->datarate = LMIC.datarate;
            pn->opmode = LMIC.opmode;
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
    Serial.println("***HALTED BY ASSERT***");
    while (true)
        yield();
}

uint8_t lastTxChannel;
bool lastTxStart;

void myEventCb(void *pUserData, ev_t ev) {
    eventQueue.putEvent(ev);

    if (ev == EV_JOINING) {
        setupForNetwork(true);
    } else if (ev == EV_JOINED) {
        setupForNetwork(false);
    }
}

void eventPrint(cEventQueue::eventnode_t &e);

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

void printFreq(u4_t freq) {
    Serial.print(F(": freq="));
    Serial.print(freq / 1000000);
    Serial.print(F("."));
    Serial.print((freq % 1000000) / 100000);
}

void printRps(rps_t rps) {
    Serial.print(F(" rps=0x")); printHex2(rps);
    Serial.print(F(" (")); Serial.print(getSfName(rps));
    Serial.print(F(" ")); Serial.print(getBwName(rps));
    Serial.print(F(" ")); Serial.print(getCrName(rps));
    Serial.print(F(" ")); Serial.print(getCrcName(rps));
    Serial.print(F(" IH=")); Serial.print(unsigned(getIh(rps)));
    Serial.print(')');
}

void printOpmode(uint16_t opmode, char sep = ',') {
    if (sep != 0)
        Serial.print(sep);
    Serial.print(F(" opmode=")); Serial.print(opmode, HEX);
}

void printTxend(ostime_t txend) {
    Serial.print(F(", txend=")); Serial.print(txend);
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

// dump all the registers.  Must have printf setup.
void printAllRegisters(void) {
    uint8_t regbuf[0x80];
    regbuf[0] = 0;
    hal_spi_read(1, regbuf + 1, sizeof(regbuf) - 1);

    for (unsigned i = 0; i < sizeof(regbuf); ++i) {
        if (i % 16 == 0) {
            printNl();
            printHex2(i);
        }
        printHex2(regbuf[i]);
        Serial.print(((i % 16) == 8) ? F(" - ") : F(" "));
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

void printNl(void) {
    Serial.println();
}

void eventPrint(cEventQueue::eventnode_t &e) {
    ev_t ev = e.event;

    Serial.print(e.time);
    Serial.print(F(" ("));
    Serial.print(osticks2ms(e.time));
    Serial.print(F(" ms): "));

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
        printTxend(e.txend);
        printTxChnl(e.txChnl);
        printRps(e.rps);
        printOpmode(e.opmode);
        printTxrxflags(e.txrxFlags);
        printSaveIrqFlags(e.saveIrqFlags);
        printAllRegisters();
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
                    Serial.print("netid: ");
                    Serial.println(netid, DEC);
                    Serial.print("devaddr: ");
                    Serial.println(devaddr, HEX);
                    Serial.print("artKey: ");
                    for (int i=0; i<sizeof(artKey); ++i) {
                        if (i != 0)
                            Serial.print("-");
                        printHex2(artKey[i]);
                    }
                    Serial.println("");
                    Serial.print("nwkKey: ");
                    for (int i=0; i<sizeof(nwkKey); ++i) {
                            if (i != 0)
                                    Serial.print("-");
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
                printf(" irqLevel %u", hal_getIrqLevel());

                printAllRegisters();
                break;

            case EV_REJOIN_FAILED:
                // this event means that someone tried a rejoin, and it failed.
                // it doesn't really mean anything bad, it's just advisory.
                break;

            case EV_TXCOMPLETE:
                printTxChnl(e.txChnl);
                printRps(e.rps);
                printTxrxflags(e.txrxFlags);
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
                printTxend(e.txend);
                break;

            case EV_RXSTART:
                printFreq(e.freq);
                printRps(e.rps);
                printDatarate(e.datarate);
                printOpmode(e.opmode);
                printTxend(e.txend);
                Serial.print(F(", delta ms ")); Serial.print(osticks2ms(e.time - e.txend));
                break;

            case EV_JOIN_TXCOMPLETE:
                printSaveIrqFlags(e.saveIrqFlags);
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

// there's a problem with running 2.5 of the MCCI STM32 BSPs;
// hack around it.
#ifdef ARDUINO_ARCH_STM32
# ifdef _mcci_arduino_version
#  if _mcci_arduino_version <= _mcci_arduino_version_calc(2, 5, 0, 0)
uint32_t USBD_LL_ConnectionState(void) {
  return 1;
}
#  endif // _mcci_arduino_version
# endif // defined(_mcci_arduino_version)
#endif // ARDUINO_ARCH_STM32

void setup() {
    delay(5000);
    while (! Serial)
        ;
    Serial.begin(115200);
    setup_printSignOn();

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

    LMIC_setClockError(5 * MAX_CLOCK_ERROR / 100);

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
    Serial.print(F("LMIC version "));
    printVersion(ARDUINO_LMIC_VERSION);
    Serial.print(F(" configured for region "));
    Serial.print(CFG_region);
    Serial.println('.');
    Serial.println(F("Remember to select 'Line Ending: Newline' at the bottom of the monitor window."));

    setup_printSignOnDashLine();
    printNl();
    }

void setupForNetwork(bool preJoin) {
#if defined(CFG_us915)
    LMIC_selectSubBand(0);

    if (! preJoin) {
//        LMIC_setLinkCheckMode(0);
//        LMIC_setDrTxpow(DR_SF7, 14);
    }
#endif
}

void loop() {
    os_runloop_once();
    while ((LMIC.opmode & OP_TXRXPEND) == 0 &&
           ! os_queryTimeCriticalJobs(ms2osticks(1000)) &&
           eventPrintOne())
        ;
}
