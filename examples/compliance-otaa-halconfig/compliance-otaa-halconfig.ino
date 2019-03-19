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
static const u1_t PROGMEM APPEUI[8]= { 1, 0, 0, 0, 0, 0, 0, 0 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
// This corresponds to 0x0000000000000001
static const u1_t PROGMEM DEVEUI[8]= { 1, 0, 0, 0, 0, 0, 0, 0 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). 
static const u1_t PROGMEM APPKEY[16] = { 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 2 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 5;

// global flag for test mode.
bool g_fTestMode = false;

// forward declarations
lmic_event_cb_t myEventCb;
lmic_rxmessage_cb_t myRxMessageCb;

const char * const evNames[] = {
    "<<zero>>",
    "EV_SCAN_TIMEOUT", "EV_BEACON_FOUND",
    "EV_BEACON_MISSED", "EV_BEACON_TRACKED", "EV_JOINING",
    "EV_JOINED", "EV_RFU1", "EV_JOIN_FAILED", "EV_REJOIN_FAILED",
    "EV_TXCOMPLETE", "EV_LOST_TSYNC", "EV_RESET",
    "EV_RXCOMPLETE", "EV_LINK_DEAD", "EV_LINK_ALIVE", "EV_SCAN_FOUND",
    "EV_TXSTART", "EV_TXCANCELED", "EV_RXSTART", "EV_JOIN_TXCOMPLETE"
};

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

    bool putEvent(ev_t event) {
        auto i = m_tail + 1;
        if (i == sizeof(m_queue) / sizeof(m_queue[0])) {
            i = 0;
        }
        if (i != m_head) {
            auto const pn = &m_queue[m_tail];
            pn->time = os_getTime();
            pn->txend = LMIC.txend;
            pn->event = event;
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
    eventnode_t m_queue[16];
    osjob_t m_job;
};

cEventQueue eventQueue;

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

void printFreq(u4_t freq) {
    Serial.print(freq / 1000000);
    Serial.print(F("."));
    Serial.print((freq % 1000000) / 100000);
}

void printRps(rps_t rps) {
    Serial.print(F(" rps=0x")); Serial.print(unsigned(rps), HEX);
    Serial.print(F(" (")); Serial.print(getSfName(rps));
    Serial.print(F(" ")); Serial.print(getBwName(rps));
    Serial.print(F(" ")); Serial.print(getCrName(rps));
    Serial.print(F(" ")); Serial.print(getCrcName(rps));
    Serial.print(F(" IH=")); Serial.print(unsigned(getIh(rps)));
    Serial.print(F(")"));
}

void eventPrint(cEventQueue::eventnode_t &e) {
    ev_t ev = e.event;

    Serial.print(e.time);
    Serial.print(F(": "));
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
            Serial.print(F(": ch "));
            Serial.println(unsigned(e.txChnl));
            {
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
                Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("nwkKey: ");
              for (int i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      Serial.print(nwkKey[i], HEX);
              }
            }
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
            Serial.print(F(":  freq=")); printFreq(e.freq);
            printRps(e.rps);
            Serial.print(F(" opmode=")); Serial.print(e.opmode, HEX);
            printf(" irqLevel %u", hal_getIrqLevel());

            do {
                uint8_t regbuf[0x80];
                regbuf[0] = 0;
                hal_spi_read(1, regbuf + 1, sizeof(regbuf) - 1);

                for (unsigned i = 0; i < sizeof(regbuf); ++i) {
                    if (i % 16 == 0) {
                        printf("\r\n%02x:", i);
                    }
                    printf("%s%02x", ((i % 16) == 8) ? " - " : " ", regbuf[i]);
                }

            hal_pin_rst(0);
            delay(2);
            hal_pin_rst(2);
            delay(6);

            const uint8_t opmode = 0x88;    // LoRa and sleep.
            hal_spi_write(0x81, &opmode, 1);

            } while (0);
            break;

        case EV_REJOIN_FAILED:
            // this event means that someone tried a rejoin, and it failed.
            // it doesn't really mean anything bad, it's just advisory.
            break;

        case EV_TXCOMPLETE:
            Serial.print(F(": ch "));
            Serial.print(e.txChnl);
            printRps(e.rps);
            Serial.print(F(" txrxflags 0x")); Serial.print(e.txrxFlags, HEX);
            if (e.txrxFlags & TXRX_ACK)
                Serial.print(F("; Received ack"));
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
            Serial.print(F(": ch "));
            Serial.print(unsigned(e.txChnl));
            printRps(e.rps);
            Serial.print(F(", datarate ")); Serial.print(unsigned(e.datarate));
            Serial.print(F(", opmode ")); Serial.print(unsigned(e.opmode), HEX);
            break;

        case EV_RXSTART:
            Serial.print(F(": freq="));
            printFreq(e.freq);
            printRps(e.rps);
            Serial.print(F(", datarate ")); Serial.print(unsigned(e.datarate));
            Serial.print(F(", opmode ")); Serial.print(unsigned(e.opmode), HEX);
            Serial.print(F(", txend ")); Serial.print(e.txend); 
            Serial.print(F(", delta ms ")); Serial.print(osticks2ms(e.time - e.txend));
            break;

        case EV_JOIN_TXCOMPLETE:
            Serial.print(F(": saveIrqFlags 0x"));
            Serial.print(unsigned(e.saveIrqFlags), HEX);
            break;

        default:
            break;
    }
    Serial.println("");
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
                Serial.print(F("Received test packet "));
                if (nMessage > 0)
                    Serial.print(pMessage[0], HEX);
                Serial.print(F(" length 0x"));
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
        if (LMIC_sendWithCallback(1, mydata, sizeof(mydata)-1, 0, sendComplete, j) == 0) {
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
    Serial.println(F("Starting"));

    // LMIC init using the computed target
    const auto pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    // don't die mysteriously; die noisily.
    if (pPinMap == nullptr) {
        myFail("board not known to library; add pinmap or update getconfig_thisboard.cpp");
    }

    // now that we have a pinmap, initalize the low levels accordingly.
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

void setupForNetwork(bool preJoin) {
#if defined(CFG_us915)
    LMIC_selectSubBand(1);

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
