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

osjob_t txjob;
osjob_t timeoutjob;
static void tx_func (osjob_t* job);

// Transmit the given string and call the given function afterwards
void tx(const char *str, osjobcb_t func) {
  os_radio(RADIO_RST); // Stop RX first
  delay(1); // Wait a bit, without this os_radio below asserts, apparently because the state hasn't changed yet
  LMIC.dataLen = 0;
  while (*str)
    LMIC.frame[LMIC.dataLen++] = *str++;
  LMIC.osjob.func = func;
  os_radio(RADIO_TX);
  Serial.println("TX");
}

// Enable rx mode and call func when a packet is received
void rx(osjobcb_t func) {
  LMIC.osjob.func = func;
  LMIC.rxtime = os_getTime(); // RX _now_
  // Enable "continuous" RX (e.g. without a timeout, still stops after
  // receiving a packet)
  os_radio(RADIO_RXON);
  Serial.println("RX");
}

static void rxtimeout_func(osjob_t *job) {
  #ifdef LED_RED
  digitalWrite(LED_RED, LOW); // on
  digitalWrite(LED_GREEN, HIGH); // off
  #else
  digitalWrite(LED_BUILTIN, LOW); // off
  #endif
}

static void rx_func (osjob_t* job) {
  #ifdef LED_RED
  digitalWrite(LED_RED, HIGH); // off
  digitalWrite(LED_GREEN, HIGH); // off
  delay(10);
  digitalWrite(LED_GREEN, LOW); // on
  #else
  digitalWrite(LED_BUILTIN, HIGH); // off
  delay(10);
  digitalWrite(LED_BUILTIN, HIGH); // on
  #endif

  // Timeout RX (i.e. update led status) after 1 second without RX
  os_setTimedCallback(&timeoutjob, os_getTime() + ms2osticks(2500), rxtimeout_func);

  // Reschedule TX so that it should not collide with the other side's
  // next TX
  os_setTimedCallback(&txjob, os_getTime() + ms2osticks(500), tx_func);

  Serial.print("Got ");
  Serial.print(LMIC.dataLen);
  Serial.println(" bytes");
  Serial.write(LMIC.frame, LMIC.dataLen);
  Serial.println();

  // Restart RX
  rx(rx_func);
}

static void txdone_func (osjob_t* job) {
  rx(rx_func);
}

// log text to USART and toggle LED
static void tx_func (osjob_t* job) {
  // say hello
  tx("Hello, world!", txdone_func);
  // reschedule job every 1000-1500ms, unless packets are received, then
  // rx_func will reschedule more often
  os_setTimedCallback(job, os_getTime() + ms2osticks(1000 + random(500)), tx_func);
}

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

  #ifdef LED_RED
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  #else
  pinMode(LED_BUILTIN, OUTPUT);
  #endif

  // initialize runtime env
  os_init();

  // Set up these settings once, and use them for both TX and RX

  // Use the ping settings. In 868Mhz, this is F6 frequency which allows
  // 10% duty cycle and the most tx power (27dBm). In 868 this is
  // channel 0, which allows up to 30dBm.
  LMIC.datarate = DR_PING;
  LMIC.freq = FREQ_PING;
  LMIC.txpow = 27;
  LMIC.rps = updr2rps(LMIC.datarate);

  Serial.println("Started");
  Serial.flush();

  // setup initial job
  os_setCallback(&txjob, tx_func);
}

void loop() {
  // execute scheduled jobs and events
  os_runloop();

  // (not reached)
}
