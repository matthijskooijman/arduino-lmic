/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

#include "Arduino.h"
#include "lmic.h"

//////////////////////////////////////////////////
// CONFIGURATION (FOR APPLICATION CALLBACKS BELOW)
//////////////////////////////////////////////////

// application router ID (LSBF)
static const u1_t APPEUI[8]  = { 0x02, 0x00, 0x00, 0x00, 0x00, 0xEE, 0xFF, 0xC0 };

// unique device ID (LSBF)
static const u1_t DEVEUI[8]  = { 0x42, 0x42, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };

static const u1_t DEVKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

static const u1_t ARTKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };


//////////////////////////////////////////////////
// APPLICATION CALLBACKS
//////////////////////////////////////////////////
extern "C" {

// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
    memcpy(buf, APPEUI, 8);
}

// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
    memcpy(buf, DEVEUI, 8);
}

// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
    memcpy(buf, DEVKEY, 16);
}


//////////////////////////////////////////////////
// MAIN - INITIALIZATION AND STARTUP
//////////////////////////////////////////////////

void onEvent (ev_t ev);
// initial job
static void initfunc (osjob_t* j) {
    // reset MAC state
    LMIC_reset();
    // Instead of OTAA joining, set static session parameters
  LMIC_setSession (0x00000065, 0x12345678, (uint8_t*)DEVKEY, (uint8_t*)ARTKEY);
  // Since we won't get a joined notification, fake one to get TX going
  onEvent(EV_JOINED);
  //  LMIC_startJoining();
    // init done - onEvent() callback will be invoked...
}


// application entry point
int do_main () {
    osjob_t initjob;

    // initialize runtime env
    os_init();
    // setup initial job
    os_setCallback(&initjob, initfunc);
    // execute scheduled jobs and events
    os_runloop();
    // (not reached)
    return 0;
}


void printframe(uint8_t *buf, size_t len);

//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////
uint8_t data[] = {0x42, 0x42};
void onEvent (ev_t ev) {
    Serial.println(millis() / 1000);
    Serial.print("EVT: ");
    Serial.println(ev);

    switch(ev) {
   
      // network joined, session established
      case EV_JOINED:
          goto tx;
        
      // scheduled data sent (optionally data received)
      case EV_TXCOMPLETE:
          LMIC_enableTracking(0);
          if(LMIC.dataLen) { // data received in rx slot after tx
              printframe(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
          }
        tx:
	  // immediately prepare next transmission
	  LMIC.frame[0] = LMIC.snr;
	  // schedule transmission (port 1, datalen 1, no ack requested)
	  LMIC_setTxData2(1, data, 2, 0);
          // (will be sent as soon as duty cycle permits)
	  break;
       default:
          break;
    }
}
}
