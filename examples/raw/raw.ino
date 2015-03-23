/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

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
  delay(1); // Wait a bit, without this os_radio below asserts
  LMIC.dataLen = 0;
  while (*str)
    LMIC.frame[LMIC.dataLen++] = *str++;
  LMIC.osjob.func = func;
  os_radio(RADIO_TX);
}

// Enable rx mode and call func when a packet is received
void rx(osjobcb_t func) {
  LMIC.osjob.func = func;
  // Enable RX continuously
  os_radio(RADIO_RXON);
}

static void rx_func (osjob_t* job) {
  Serial.print("RX ");
  Serial.print(LMIC.dataLen);
  Serial.println(" bytes");
  Serial.write(LMIC.frame, LMIC.dataLen);
  Serial.println();
}

static void txdone_func (osjob_t* job) {
  rx(rx_func);
}

// log text to USART and toggle LED
static void init_func (osjob_t* job) {
  // say hello
  Serial.println("TX");
  Serial.flush();
  tx("Hello, world!", txdone_func);
  // reschedule job every second
  os_setTimedCallback(job, os_getTime() + sec2osticks(1), init_func);
}

osjob_t initjob;

// application entry point
void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  #ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
  #endif

  // initialize runtime env
  os_init();

  Serial.println("Started");
  Serial.flush();

  // setup initial job
  os_setCallback(&initjob, init_func);
}

void loop() {
  // execute scheduled jobs and events
  os_runloop();

  // (not reached)
}
