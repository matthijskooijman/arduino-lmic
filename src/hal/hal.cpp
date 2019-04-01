/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 * Copyright (c) 2018-2019 MCCI Corporation
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
// include all the lmic header files, including ../lmic/hal.h
#include "../lmic.h"
// include the C++ hal.h
#include "hal.h"
// we may need some things from stdio.
#include <stdio.h>

// -----------------------------------------------------------------------------
// I/O

static const Arduino_LMIC::HalPinmap_t *plmic_pins;
static Arduino_LMIC::HalConfiguration_t *pHalConfig;
static Arduino_LMIC::HalConfiguration_t nullHalConig;
static hal_failure_handler_t* custom_hal_failure_handler = NULL;

static void hal_interrupt_init(); // Fwd declaration

static void hal_io_init () {
    // NSS and DIO0 are required, DIO1 is required for LoRa, DIO2 for FSK
    ASSERT(plmic_pins->nss != LMIC_UNUSED_PIN);
    ASSERT(plmic_pins->dio[0] != LMIC_UNUSED_PIN);
    ASSERT(plmic_pins->dio[1] != LMIC_UNUSED_PIN || plmic_pins->dio[2] != LMIC_UNUSED_PIN);

//    Serial.print("nss: "); Serial.println(plmic_pins->nss);
//    Serial.print("rst: "); Serial.println(plmic_pins->rst);
//    Serial.print("dio[0]: "); Serial.println(plmic_pins->dio[0]);
//    Serial.print("dio[1]: "); Serial.println(plmic_pins->dio[1]);
//    Serial.print("dio[2]: "); Serial.println(plmic_pins->dio[2]);

    // initialize SPI chip select to high (it's active low)
    digitalWrite(plmic_pins->nss, HIGH);
    pinMode(plmic_pins->nss, OUTPUT);

    if (plmic_pins->rxtx != LMIC_UNUSED_PIN) {
        // initialize to RX
        digitalWrite(plmic_pins->rxtx, LOW != plmic_pins->rxtx_rx_active);
        pinMode(plmic_pins->rxtx, OUTPUT);
    }
    if (plmic_pins->rst != LMIC_UNUSED_PIN) {
        // initialize RST to floating
        pinMode(plmic_pins->rst, INPUT);
    }

    hal_interrupt_init();
}

// val == 1  => tx
void hal_pin_rxtx (u1_t val) {
    if (plmic_pins->rxtx != LMIC_UNUSED_PIN)
        digitalWrite(plmic_pins->rxtx, val != plmic_pins->rxtx_rx_active);
}

// set radio RST pin to given value (or keep floating!)
void hal_pin_rst (u1_t val) {
    if (plmic_pins->rst == LMIC_UNUSED_PIN)
        return;

    if(val == 0 || val == 1) { // drive pin
        digitalWrite(plmic_pins->rst, val);
        pinMode(plmic_pins->rst, OUTPUT);
    } else { // keep pin floating
        pinMode(plmic_pins->rst, INPUT);
    }
}

s1_t hal_getRssiCal (void) {
    return plmic_pins->rssi_cal;
}

#if !defined(LMIC_USE_INTERRUPTS)
static void hal_interrupt_init() {
    pinMode(plmic_pins->dio[0], INPUT);
    if (plmic_pins->dio[1] != LMIC_UNUSED_PIN)
        pinMode(plmic_pins->dio[1], INPUT);
    if (plmic_pins->dio[2] != LMIC_UNUSED_PIN)
        pinMode(plmic_pins->dio[2], INPUT);
}

static bool dio_states[NUM_DIO] = {0};
static void hal_io_check() {
    uint8_t i;
    for (i = 0; i < NUM_DIO; ++i) {
        if (plmic_pins->dio[i] == LMIC_UNUSED_PIN)
            continue;

        if (dio_states[i] != digitalRead(plmic_pins->dio[i])) {
            dio_states[i] = !dio_states[i];
            if (dio_states[i])
                radio_irq_handler(i);
        }
    }
}

#else
// Interrupt handlers
static ostime_t interrupt_time[NUM_DIO] = {0};

static void hal_isrPin0() {
    ostime_t now = os_getTime();
    interrupt_time[0] = now ? now : 1;
}
static void hal_isrPin1() {
    ostime_t now = os_getTime();
    interrupt_time[1] = now ? now : 1;
}
static void hal_isrPin2() {
    ostime_t now = os_getTime();
    interrupt_time[2] = now ? now : 1;
}

typedef void (*isr_t)();
static isr_t interrupt_fns[NUM_DIO] = {hal_isrPin0, hal_isrPin1, hal_isrPin2};

static void hal_interrupt_init() {
  for (uint8_t i = 0; i < NUM_DIO; ++i) {
      if (plmic_pins->dio[i] == LMIC_UNUSED_PIN)
          continue;

      attachInterrupt(digitalPinToInterrupt(plmic_pins->dio[i]), interrupt_fns[i], RISING);
  }
}

static void hal_io_check() {
    uint8_t i;
    for (i = 0; i < NUM_DIO; ++i) {
        ostime_t iTime;
        if (plmic_pins->dio[i] == LMIC_UNUSED_PIN)
            continue;

        iTime = interrupt_time[i];
        if (iTime) {
            interrupt_time[i] = 0;
            radio_irq_handler_v2(i, iTime);
        }
    }
}
#endif // LMIC_USE_INTERRUPTS

// -----------------------------------------------------------------------------
// SPI

static void hal_spi_init () {
    SPI.begin();
}

static void hal_spi_trx(u1_t cmd, u1_t* buf, size_t len, bit_t is_read) {
    uint32_t spi_freq;
    u1_t nss = plmic_pins->nss;

    if ((spi_freq = plmic_pins->spi_freq) == 0)
        spi_freq = LMIC_SPI_FREQ;

    SPISettings settings(spi_freq, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(settings);
    digitalWrite(nss, 0);

    SPI.transfer(cmd);

    for (; len > 0; --len, ++buf) {
        u1_t data = is_read ? 0x00 : *buf;
        data = SPI.transfer(data);
        if (is_read)
            *buf = data;
    }

    digitalWrite(nss, 1);
    SPI.endTransaction();
}

void hal_spi_write(u1_t cmd, const u1_t* buf, size_t len) {
    hal_spi_trx(cmd, (u1_t*)buf, len, 0);
}

void hal_spi_read(u1_t cmd, u1_t* buf, size_t len) {
    hal_spi_trx(cmd, buf, len, 1);
}

// -----------------------------------------------------------------------------
// TIME

static void hal_time_init () {
    // Nothing to do
}

u4_t hal_ticks () {
    // Because micros() is scaled down in this function, micros() will
    // overflow before the tick timer should, causing the tick timer to
    // miss a significant part of its values if not corrected. To fix
    // this, the "overflow" serves as an overflow area for the micros()
    // counter. It consists of three parts:
    //  - The US_PER_OSTICK upper bits are effectively an extension for
    //    the micros() counter and are added to the result of this
    //    function.
    //  - The next bit overlaps with the most significant bit of
    //    micros(). This is used to detect micros() overflows.
    //  - The remaining bits are always zero.
    //
    // By comparing the overlapping bit with the corresponding bit in
    // the micros() return value, overflows can be detected and the
    // upper bits are incremented. This is done using some clever
    // bitwise operations, to remove the need for comparisons and a
    // jumps, which should result in efficient code. By avoiding shifts
    // other than by multiples of 8 as much as possible, this is also
    // efficient on AVR (which only has 1-bit shifts).
    static uint8_t overflow = 0;

    // Scaled down timestamp. The top US_PER_OSTICK_EXPONENT bits are 0,
    // the others will be the lower bits of our return value.
    uint32_t scaled = micros() >> US_PER_OSTICK_EXPONENT;
    // Most significant byte of scaled
    uint8_t msb = scaled >> 24;
    // Mask pointing to the overlapping bit in msb and overflow.
    const uint8_t mask = (1 << (7 - US_PER_OSTICK_EXPONENT));
    // Update overflow. If the overlapping bit is different
    // between overflow and msb, it is added to the stored value,
    // so the overlapping bit becomes equal again and, if it changed
    // from 1 to 0, the upper bits are incremented.
    overflow += (msb ^ overflow) & mask;

    // Return the scaled value with the upper bits of stored added. The
    // overlapping bit will be equal and the lower bits will be 0, so
    // bitwise or is a no-op for them.
    return scaled | ((uint32_t)overflow << 24);

    // 0 leads to correct, but overly complex code (it could just return
    // micros() unmodified), 8 leaves no room for the overlapping bit.
    static_assert(US_PER_OSTICK_EXPONENT > 0 && US_PER_OSTICK_EXPONENT < 8, "Invalid US_PER_OSTICK_EXPONENT value");
}

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static s4_t delta_time(u4_t time) {
    return (s4_t)(time - hal_ticks());
}

void hal_waitUntil (u4_t time) {
    s4_t delta = delta_time(time);
    // From delayMicroseconds docs: Currently, the largest value that
    // will produce an accurate delay is 16383.
    while (delta > (16000 / US_PER_OSTICK)) {
        delay(16);
        delta -= (16000 / US_PER_OSTICK);
    }
    if (delta > 0)
        delayMicroseconds(delta * US_PER_OSTICK);
}

// check and rewind for target time
u1_t hal_checkTimer (u4_t time) {
    // No need to schedule wakeup, since we're not sleeping
    return delta_time(time) <= 0;
}

static uint8_t irqlevel = 0;

void hal_disableIRQs () {
    noInterrupts();
    irqlevel++;
}

void hal_enableIRQs () {
    if(--irqlevel == 0) {
        interrupts();

        // Instead of using proper interrupts (which are a bit tricky
        // and/or not available on all pins on AVR), just poll the pin
        // values. Since os_runloop disables and re-enables interrupts,
        // putting this here makes sure we check at least once every
        // loop.
        //
        // As an additional bonus, this prevents the can of worms that
        // we would otherwise get for running SPI transfers inside ISRs
        hal_io_check();
    }
}

uint8_t hal_getIrqLevel(void) {
    return irqlevel;
}

void hal_sleep () {
    // Not implemented
}

// -----------------------------------------------------------------------------

#if defined(LMIC_PRINTF_TO)
#if !defined(__AVR)
static ssize_t uart_putchar (void *, const char *buf, size_t len) {
    return LMIC_PRINTF_TO.write((const uint8_t *)buf, len);
}

static cookie_io_functions_t functions =
 {
     .read = NULL,
     .write = uart_putchar,
     .seek = NULL,
     .close = NULL
 };

void hal_printf_init() {
    stdout = fopencookie(NULL, "w", functions);
    if (stdout != nullptr) {
        setvbuf(stdout, NULL, _IONBF, 0);
    }
}
#else // defined(__AVR)
static int uart_putchar (char c, FILE *)
{
    LMIC_PRINTF_TO.write(c) ;
    return 0 ;
}

void hal_printf_init() {
    // create a FILE structure to reference our UART output function
    static FILE uartout;
    memset(&uartout, 0, sizeof(uartout));

    // fill in the UART file descriptor with pointer to writer.
    fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);

    // The uart is the standard output device STDOUT.
    stdout = &uartout ;
}

#endif // !defined(ESP8266) || defined(ESP31B) || defined(ESP32)
#endif // defined(LMIC_PRINTF_TO)

void hal_init (void) {
    // use the global constant
    Arduino_LMIC::hal_init_with_pinmap(&lmic_pins);
}

// hal_init_ex is a C API routine, written in C++, and it's called
// with a pointer to an lmic_pinmap.
void hal_init_ex (const void *pContext) {
    const lmic_pinmap * const pHalPinmap = (const lmic_pinmap *) pContext;
    if (! Arduino_LMIC::hal_init_with_pinmap(pHalPinmap)) {
        hal_failed(__FILE__, __LINE__);
    }
}

// C++ API: initialize the HAL properly with a configuration object
namespace Arduino_LMIC {
bool hal_init_with_pinmap(const HalPinmap_t *pPinmap)
    {
    if (pPinmap == nullptr)
        return false;

    // set the static pinmap pointer.
    plmic_pins = pPinmap;

    // set the static HalConfiguration pointer.
    HalConfiguration_t * const pThisHalConfig = pPinmap->pConfig;

    if (pThisHalConfig != nullptr)
        pHalConfig = pThisHalConfig;
    else
        pHalConfig = &nullHalConig;

    pHalConfig->begin();

    // configure radio I/O and interrupt handler
    hal_io_init();
    // configure radio SPI
    hal_spi_init();
    // configure timer and interrupt handler
    hal_time_init();
#if defined(LMIC_PRINTF_TO)
    // printf support
    hal_printf_init();
#endif
    // declare success
    return true;
    }
}; // namespace Arduino_LMIC


void hal_failed (const char *file, u2_t line) {
    if (custom_hal_failure_handler != NULL) {
        (*custom_hal_failure_handler)(file, line);
    }

#if defined(LMIC_FAILURE_TO)
    LMIC_FAILURE_TO.println("FAILURE ");
    LMIC_FAILURE_TO.print(file);
    LMIC_FAILURE_TO.print(':');
    LMIC_FAILURE_TO.println(line);
    LMIC_FAILURE_TO.flush();
#endif

    hal_disableIRQs();

    // Infinite loop
    while (1) {
        ;
    }
}

void hal_set_failure_handler(const hal_failure_handler_t* const handler) {
    custom_hal_failure_handler = handler;
}

ostime_t hal_setModuleActive (bit_t val) {
    // setModuleActive() takes a c++ bool, so
    // it effectively says "val != 0". We
    // don't have to.
    return pHalConfig->setModuleActive(val);
}

bit_t hal_queryUsingTcxo(void) {
    return pHalConfig->queryUsingTcxo();
}