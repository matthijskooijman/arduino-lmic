/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *******************************************************************************/

// This is the Pinoccio Scout bootstrap sketch, with a LoRa tx()
// bitlash function added. When not transmitting, rx is enabled.
// Compiling this sketch requires the Pinoccio core and the full set of
// Pinoccio libraries.
//
// Don't bother trying this sketch if you are not using the Pinoccio
// Scout board!

#include <SPI.h>
#include <Wire.h>
#include <Scout.h>
#include <GS.h>
#include <bitlash.h>
#include <lwm.h>
#include <js0n.h>

#include <lmic.h>
#include <hal/hal.h>

lmic_pinmap pins = {
  .nss = SS,
  .rxtx = 8, // Not connected
  .rst = A7, // Not connected
  .dio = {4, 5, 7},
};

//LMIC application callbacks not used in his example
void os_getArtEui (u1_t* buf) {
}

void os_getDevEui (u1_t* buf) {
}

void os_getDevKey (u1_t* buf) {
}

void onEvent (ev_t ev) {
}

// Transmit the given string and call the given function afterwards
void tx(const char *str, osjobcb_t func) {
  os_radio(RADIO_RST); // Stop RX first
  delay(1); // Wait a bit, without this os_radio below asserts, apparently because the state hasn't changed yet
  LMIC.dataLen = 0;
  while (*str)
    LMIC.frame[LMIC.dataLen++] = *str++;
  LMIC.osjob.func = func;
  os_radio(RADIO_TX);
}

// Enable rx mode and call func when a packet is received
void rx(osjobcb_t func) {
  LMIC.osjob.func = func;
  LMIC.rxtime = os_getTime(); // RX _now_
  // Enable "continuous" RX (e.g. without a timeout, still stops after
  // receiving a packet)
  os_radio(RADIO_RXON);
}

static void rx_func (osjob_t* job) {
  Serial.print("LoRa ");
  Serial.print(LMIC.dataLen);
  Serial.print(" bytes: ");
  Serial.write(LMIC.frame, LMIC.dataLen);
  Serial.println();

  // Restart RX
  rx(rx_func);
}

static void txdone_func (osjob_t* job) {
  // Continue RX
  rx(rx_func);
}

numvar transmit() {
  if (getarg(0) != 1 || !isstringarg(1))
    speol(F("Usage: tx(\"string to send\")"));
  else
    tx((const char*)getstringarg(1), txdone_func);
  return 0;
}

// application entry point
void setup() {
  Scout.setup("LoRa test", "N/A", 0);

  // Setup LoRa
  os_init();

  // Set up these settings once, and use them for both TX and RX

  // Use the ping settings. In 868Mhz, this is F6 frequency which allows
  // 10% duty cycle and the most tx power (27dBm). In 868 this is
  // channel 0, which allows up to 30dBm.
  LMIC.datarate = DR_PING;
  LMIC.freq = FREQ_PING;
  LMIC.txpow = 27;
  LMIC.rps = updr2rps(LMIC.datarate);

  addBitlashFunction("tx", transmit);

  // Start rx
  rx(rx_func);
}

void loop() {
  Scout.loop();

  // Run LoRa jobs
  os_runloop_once();
}
