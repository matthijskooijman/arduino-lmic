/*******************************************************************************
 * Copyright (c) 2019 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 *******************************************************************************/

#include <arduino_lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <arduino_lmic_hal_boards.h>
#include <arduino_lmic_lorawan_cert.h>

//
// For certification tests with the RWC5020A, we use the default addresses
// from the tester.
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
const unsigned TX_INTERVAL = 60;

// global flag for test mode.
bool g_fTestMode = false;

// forward declarations
lmic_event_cb_t myEventCb;
lmic_rxmessage_cb_t myRxMessageCb;


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

void myEventCb(void *pUserData, ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
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
              Serial.println("");
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
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
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
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
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
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
        certification handler (if certification support is needed), and
        then decode any non-certification messages.

Returns:
        No explicit result.

*/

void myRxMessageCb(
    void *pUserData,
    uint8_t port,
    const uint8_t *pMessage,
    size_t nMessage
) {
    lmic_cert_rx_action_t const action = LMIC_certRxMessage(port, pMessage, nMessage);
    switch (action) {
        case LMIC_CERT_RX_ACTION_START: {
            Serial.println(F("Enter test mode"));
            os_clearCallback(&sendjob);
            g_fTestMode = true;
            return;
        }
        case LMIC_CERT_RX_ACTION_END: {
            Serial.println(F("Exit test mode"));
            g_fTestMode = false;
            do_send(&sendjob);
            return;
        }
        case LMIC_CERT_RX_ACTION_IGNORE: {
            if (port == LORAWAN_PORT_CERT) {
                Serial.print(F("Received test packet "));
                if (nMessage > 0)
                    Serial.print(pMessage[0], HEX);
                Serial.print(F(" length 0x"));
                Serial.println(nMessage);
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
    Serial.print(nMessage);
    Serial.println(F(" bytes"));
    }

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
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
    const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    // don't die mysteriously; die noisily.
    if (pPinMap == nullptr) {
        myFail("board not known to library; add pinmap or update getconfig_thisboard.cpp");
    }

    // now that we have a pinmap, initalize the low levels accordingly.
    os_init_ex(pPinMap);

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Reset resets *everything*, so we need to register after the LMIC_reset().
    if (! (LMIC_registerRxMessageCb(myRxMessageCb, /* userData */ nullptr) && 
           LMIC_registerEventCb(myEventCb, /* userData */ nullptr))) {
        myFail("couldn't register callbacks");
    }

    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7, 14);
    LMIC_selectSubBand(1);

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
