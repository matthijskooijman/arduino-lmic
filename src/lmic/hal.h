/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * Copyright (c) 2016, 2018-2019 MCCI Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _lmic_hal_h_
#define _lmic_hal_h_

#ifndef _oslmic_types_h_
# include "oslmic_types.h"
#endif

#ifndef _lmic_env_h_
# include "lmic_env.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

// The type of an optional user-defined failure handler routine
typedef void LMIC_ABI_STD hal_failure_handler_t(const char* const file, const uint16_t line);

/*
 * initialize hardware (IO, SPI, TIMER, IRQ).
 * This API is deprecated as it uses the const global lmic_pins,
 * which the platform can't control or change.
 */
void hal_init (void);

/*
 * Initialize hardware, passing in platform-specific context
 * The pointer is to a HalPinmap_t.
 */
void hal_init_ex (const void *pContext);

/*
 * drive radio RX/TX pins (0=rx, 1=tx). Actual polarity
 * is determined by the value of HalPinmap_t::rxtx_rx_active.
 */
void hal_pin_rxtx (u1_t val);

/*
 * control radio RST pin (0=low, 1=high, 2=floating)
 */
void hal_pin_rst (u1_t val);

/*
 * Perform SPI write transaction with radio chip
 *   - write the command byte 'cmd'
 *   - write 'len' bytes out of 'buf'
 */
void hal_spi_write(u1_t cmd, const u1_t* buf, size_t len);

/*
 * Perform SPI read transaction with radio chip
 *   - write the command byte 'cmd'
 *   - read 'len' bytes into 'buf'
 */
void hal_spi_read(u1_t cmd, u1_t* buf, size_t len);

/*
 * disable all CPU interrupts.
 *   - might be invoked nested
 *   - will be followed by matching call to hal_enableIRQs()
 */
void hal_disableIRQs (void);

/*
 * enable CPU interrupts.
 */
void hal_enableIRQs (void);

/*
 * return CPU interrupt nesting count
 */
uint8_t hal_getIrqLevel (void);

/*
 * put system and CPU in low-power mode, sleep until interrupt.
 */
void hal_sleep (void);

/*
 * return 32-bit system time in ticks.
 */
u4_t hal_ticks (void);

/*
 * busy-wait until specified timestamp (in ticks) is reached. If on-time, return 0,
 * otherwise return the number of ticks we were late.
 */
u4_t hal_waitUntil (u4_t time);

/*
 * check and rewind timer for target time.
 *   - return 1 if target time is close
 *   - otherwise rewind timer for target time or full period and return 0
 */
u1_t hal_checkTimer (u4_t targettime);

/*
 * perform fatal failure action.
 *   - called by assertions
 *   - action could be HALT or reboot
 */
void hal_failed (const char *file, u2_t line);

/*
 * set a custom hal failure handler routine. The default behaviour, defined in
 * hal_failed(), is to halt by looping infintely.
 */
void hal_set_failure_handler(const hal_failure_handler_t* const);

/*
 * get the calibration value for radio_rssi
 */
s1_t hal_getRssiCal (void);

/*
 * control the radio state
 *   - if val == 0, turn tcxo off and otherwise prepare for sleep
 *   - if val == 1, turn tcxo on and otherwise prep for activity
 *   - return the number of ticks that we need to wait
 */
ostime_t hal_setModuleActive (bit_t val);

/* find out if we're using Tcxo */
bit_t hal_queryUsingTcxo(void);

/* represent the various radio TX power policy */
enum	{
	LMICHAL_radio_tx_power_policy_rfo	= 0,
	LMICHAL_radio_tx_power_policy_paboost	= 1,
	LMICHAL_radio_tx_power_policy_20dBm	= 2,
};

/*
 * query the configuration as to the Tx Power Policy
 * to be used on this board, given our desires and
 * requested power.
 */
uint8_t hal_getTxPowerPolicy(
	u1_t inputPolicy,
	s1_t requestedPower,
	u4_t freq
	);

void hal_pollPendingIRQs_helper();
void hal_processPendingIRQs(void);

/// \brief check for any pending interrupts: stub if interrupts are enabled.
static void inline hal_pollPendingIRQs(void)
	{
#if !defined(LMIC_USE_INTERRUPTS)
	hal_pollPendingIRQs_helper();
#endif /* !defined(LMIC_USE_INTERRUPTS) */
	}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _lmic_hal_h_
