 *******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman and Thomas Telkamp
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 *
 * WARNING: This sketch almost certainly violates duty cycle limits,
 * since it bypasses the LoRaWAN stuff and directly talks to the
 * transceiver.
 *******************************************************************************


#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>


#define LED_BUILTIN 8

static const u1_t APPEUI[8]  = { 0x02, 0x00, 0x00, 0x00, 0x00, 0xEE, 0xFF, 0xC0 };

// unique device ID (LSBF)
static const u1_t DEVEUI[8]  = { 0x42, 0x42, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };

static const u1_t DEVKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

static const u1_t ARTKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };


//////////////////////////////////////////////////
// APPLICATION CALLBACKS
//////////////////////////////////////////////////

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

uint8_t mydata[] = "Hello, world!";
u4_t mycount = 0;

// Pin mapping
lmic_pinmap pins = {
  .nss = 10,
  .rxtx = 7, // Not connected
  .rst = 9, 
  .dio = {2, 5, 6},
};

void onEvent (ev_t ev) {} ;

void do_send(){
    LMIC_reset();
    
    LMIC_setSession (0x0, 0x0000EF01, (uint8_t*)DEVKEY, (uint8_t*)ARTKEY);
    
    LMIC_setDrTxpow(DR_SF12,0);
    LMIC_setAdrMode(0);
    
    mycount++;
    LMIC.seqnoUp=mycount;
    //LMIC.freq = EU868_F2;
    Serial.println(millis() / 1000);
    Serial.print("Send\n");
    LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
    //
    os_runloop_once();
}


void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  pinMode(LED_BUILTIN, OUTPUT); 
  os_init();
  LMIC_reset();
}


void loop() {
   do_send();
   
   delay(10000);
  
  }

