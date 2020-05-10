/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * Copyright (c) 2016-2019 MCCI Corporation.
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

//! \file

#define LMIC_DR_LEGACY 0

#include "lmic.h"

// ----------------------------------------
// Registers Mapping
//                                                      // -type-       1272 vs 1276
#define RegFifo                                    0x00 // common
#define RegOpMode                                  0x01 // common       see below
#define FSKRegBitrateMsb                           0x02 //              -
#define FSKRegBitrateLsb                           0x03 //              -
#define FSKRegFdevMsb                              0x04 //              -
#define FSKRegFdevLsb                              0x05 //              -
#define RegFrfMsb                                  0x06 // common       FSK: 1272: 915; 1276: 434 MHz
#define RegFrfMid                                  0x07 // common       ditto
#define RegFrfLsb                                  0x08 // common       ditto
#define RegPaConfig                                0x09 // common       see below, many diffs
#define RegPaRamp                                  0x0A // common       see below: bits 6..4 are diff
#define RegOcp                                     0x0B // common       -
#define RegLna                                     0x0C // common       bits 4..0 are diff.
#define FSKRegRxConfig                             0x0D //              -
#define LORARegFifoAddrPtr                         0x0D
#define FSKRegRssiConfig                           0x0E //              -
#define LORARegFifoTxBaseAddr                      0x0E
#define FSKRegRssiCollision                        0x0F //              -
#define LORARegFifoRxBaseAddr                      0x0F
#define FSKRegRssiThresh                           0x10 //              -
#define LORARegFifoRxCurrentAddr                   0x10
#define FSKRegRssiValue                            0x11 //              -
#define LORARegIrqFlagsMask                        0x11
#define FSKRegRxBw                                 0x12 //              -
#define LORARegIrqFlags                            0x12
#define FSKRegAfcBw                                0x13 //              -
#define LORARegRxNbBytes                           0x13
#define FSKRegOokPeak                              0x14 //              -
#define LORARegRxHeaderCntValueMsb                 0x14
#define FSKRegOokFix                               0x15 //              -
#define LORARegRxHeaderCntValueLsb                 0x15
#define FSKRegOokAvg                               0x16 //              -
#define LORARegRxPacketCntValueMsb                 0x16
#define LORARegRxpacketCntValueLsb                 0x17
#define LORARegModemStat                           0x18
#define LORARegPktSnrValue                         0x19
#define FSKRegAfcFei                               0x1A //              -
#define LORARegPktRssiValue                        0x1A
#define FSKRegAfcMsb                               0x1B //              -
#define LORARegRssiValue                           0x1B
#define FSKRegAfcLsb                               0x1C //              -
#define LORARegHopChannel                          0x1C
#define FSKRegFeiMsb                               0x1D //              -
#define LORARegModemConfig1                        0x1D
#define FSKRegFeiLsb                               0x1E //              -
#define LORARegModemConfig2                        0x1E
#define FSKRegPreambleDetect                       0x1F //              -
#define LORARegSymbTimeoutLsb                      0x1F
#define FSKRegRxTimeout1                           0x20 //              -
#define LORARegPreambleMsb                         0x20
#define FSKRegRxTimeout2                           0x21 //              -
#define LORARegPreambleLsb                         0x21
#define FSKRegRxTimeout3                           0x22 //              -
#define LORARegPayloadLength                       0x22
#define FSKRegRxDelay                              0x23 //              -
#define LORARegPayloadMaxLength                    0x23
#define FSKRegOsc                                  0x24 //              -
#define LORARegHopPeriod                           0x24
#define FSKRegPreambleMsb                          0x25 //              -
#define LORARegFifoRxByteAddr                      0x25
#define FSKRegPreambleLsb                          0x26 //              -
#define LORARegModemConfig3                        0x26
#define FSKRegSyncConfig                           0x27 //              -
#define LORARegFeiMsb                              0x28
#define FSKRegSyncValue1                           0x28 //              -
#define LORAFeiMib                                 0x29
#define FSKRegSyncValue2                           0x29 //              -
#define LORARegFeiLsb                              0x2A
#define FSKRegSyncValue3                           0x2A //              -
#define FSKRegSyncValue4                           0x2B //              -
#define LORARegRssiWideband                        0x2C
#define FSKRegSyncValue5                           0x2C //              -
#define FSKRegSyncValue6                           0x2D //              -
#define FSKRegSyncValue7                           0x2E //              -
#define FSKRegSyncValue8                           0x2F //              -
#define LORARegIffReq1                             0x2F
#define FSKRegPacketConfig1                        0x30 //              -
#define LORARegIffReq2                             0x30
#define FSKRegPacketConfig2                        0x31 //              -
#define LORARegDetectOptimize                      0x31
#define FSKRegPayloadLength                        0x32 //              -
#define FSKRegNodeAdrs                             0x33 //              -
#define LORARegInvertIQ                            0x33
#define FSKRegBroadcastAdrs                        0x34 //              -
#define FSKRegFifoThresh                           0x35 //              -
#define FSKRegSeqConfig1                           0x36 //              -
#define LORARegHighBwOptimize1                     0x36
#define FSKRegSeqConfig2                           0x37 //              -
#define LORARegDetectionThreshold                  0x37
#define FSKRegTimerResol                           0x38 //              -
#define FSKRegTimer1Coef                           0x39 //              -
#define LORARegSyncWord                            0x39
#define FSKRegTimer2Coef                           0x3A //              -
#define LORARegHighBwOptimize2                     0x3A
#define FSKRegImageCal                             0x3B //              -
#define FSKRegTemp                                 0x3C //              -
#define FSKRegLowBat                               0x3D //              -
#define FSKRegIrqFlags1                            0x3E //              -
#define FSKRegIrqFlags2                            0x3F //              -
#define RegDioMapping1                             0x40 // common
#define RegDioMapping2                             0x41 // common
#define RegVersion                                 0x42 // common
// #define RegAgcRef                                  0x43 // common
// #define RegAgcThresh1                              0x44 // common
// #define RegAgcThresh2                              0x45 // common
// #define RegAgcThresh3                              0x46 // common
// #define RegPllHop                                  0x4B // common
// #define RegTcxo                                    0x58 // common
// #define RegPll                                     0x5C // common
// #define RegPllLowPn                                0x5E // common
// #define RegFormerTemp                              0x6C // common
// #define RegBitRateFrac                             0x70 // common

#if defined(CFG_sx1276_radio)
#define RegTcxo                                    0x4B // common       different addresses, same bits
#define RegPaDac                                   0x4D // common       differnet addresses, same bits
#elif defined(CFG_sx1272_radio)
#define RegTcxo                                    0x58 // common
#define RegPaDac                                   0x5A // common
#endif

#define RegTcxo_TcxoInputOn                        (1u << 4)

// ----------------------------------------
// spread factors and mode for RegModemConfig2
#define SX1272_MC2_FSK  0x00
#define SX1272_MC2_SF7  0x70
#define SX1272_MC2_SF8  0x80
#define SX1272_MC2_SF9  0x90
#define SX1272_MC2_SF10 0xA0
#define SX1272_MC2_SF11 0xB0
#define SX1272_MC2_SF12 0xC0
// bandwidth for RegModemConfig1
#define SX1272_MC1_BW_125  0x00
#define SX1272_MC1_BW_250  0x40
#define SX1272_MC1_BW_500  0x80
// coding rate for RegModemConfig1
#define SX1272_MC1_CR_4_5 0x08
#define SX1272_MC1_CR_4_6 0x10
#define SX1272_MC1_CR_4_7 0x18
#define SX1272_MC1_CR_4_8 0x20
#define SX1272_MC1_IMPLICIT_HEADER_MODE_ON 0x04 // required for receive
#define SX1272_MC1_RX_PAYLOAD_CRCON        0x02
#define SX1272_MC1_LOW_DATA_RATE_OPTIMIZE  0x01 // mandated for SF11 and SF12
// transmit power configuration for RegPaConfig
#define SX1272_PAC_PA_SELECT_PA_BOOST 0x80
#define SX1272_PAC_PA_SELECT_RFIO_PIN 0x00


// sx1276 RegModemConfig1
#define SX1276_MC1_BW_125                0x70
#define SX1276_MC1_BW_250                0x80
#define SX1276_MC1_BW_500                0x90
#define SX1276_MC1_CR_4_5            0x02
#define SX1276_MC1_CR_4_6            0x04
#define SX1276_MC1_CR_4_7            0x06
#define SX1276_MC1_CR_4_8            0x08

#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON    0x01

#ifdef CFG_sx1276_radio
# define SX127X_MC1_IMPLICIT_HEADER_MODE_ON	SX1276_MC1_IMPLICIT_HEADER_MODE_ON
#else
# define SX127X_MC1_IMPLICIT_HEADER_MODE_ON	SX1272_MC1_IMPLICIT_HEADER_MODE_ON
#endif

// transmit power configuration for RegPaConfig
#define SX1276_PAC_PA_SELECT_PA_BOOST 0x80
#define SX1276_PAC_PA_SELECT_RFIO_PIN 0x00
#define SX1276_PAC_MAX_POWER_MASK     0x70

// the bits to change for max power.
#define SX127X_PADAC_POWER_MASK       0x07
#define SX127X_PADAC_POWER_NORMAL     0x04
#define SX127X_PADAC_POWER_20dBm      0x07

// convert milliamperes to equivalent value for
// RegOcp; delivers conservative value.
#define SX127X_OCP_MAtoBITS(mA)                 \
    ((mA) < 45   ? 0 :                          \
     (mA) <= 120 ? ((mA) - 45) / 5 :            \
     (mA) < 130  ? 0xF :                        \
     (mA) < 240  ? ((mA) - 130) / 10 + 0x10 :   \
                   27)

// bit in RegOcp that enables overcurrent protect.
#define SX127X_OCP_ENA                     0x20

// sx1276 RegModemConfig2
#define SX1276_MC2_RX_PAYLOAD_CRCON        0x04

// sx1276 RegModemConfig3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE  0x08
#define SX1276_MC3_AGCAUTO                 0x04

// preamble for lora networks (nibbles swapped)
#define LORA_MAC_PREAMBLE                  0x34

#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1 0x0A
#ifdef CFG_sx1276_radio
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 0x70
#elif CFG_sx1272_radio
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 0x74
#endif

//-----------------------------------------
// Parameters for RSSI monitoring
#define SX127X_FREQ_LF_MAX      525000000       // per datasheet 6.3

// per datasheet 5.5.3 and 5.5.5:
#define SX1272_RSSI_ADJUST      -139            // add to rssi value to get dB (LF)

// per datasheet 5.5.3 and 5.5.5:
#define SX1276_RSSI_ADJUST_LF   -164            // add to rssi value to get dB (LF)
#define SX1276_RSSI_ADJUST_HF   -157            // add to rssi value to get dB (HF)

#ifdef CFG_sx1276_radio
# define SX127X_RSSI_ADJUST_LF  SX1276_RSSI_ADJUST_LF
# define SX127X_RSSI_ADJUST_HF  SX1276_RSSI_ADJUST_HF
#else
# define SX127X_RSSI_ADJUST_LF  SX1272_RSSI_ADJUST
# define SX127X_RSSI_ADJUST_HF  SX1272_RSSI_ADJUST
#endif

// per datasheet 2.5.2 (but note that we ought to ask Semtech to confirm, because
// datasheet is unclear).
#define SX127X_RX_POWER_UP      us2osticks(500) // delay this long to let the receiver power up.

// ----------------------------------------
// Constants for radio registers
#define OPMODE_LORA      0x80
#define OPMODE_MASK      0x07
#define OPMODE_SLEEP     0x00
#define OPMODE_STANDBY   0x01
#define OPMODE_FSTX      0x02
#define OPMODE_TX        0x03
#define OPMODE_FSRX      0x04
#define OPMODE_RX        0x05
#define OPMODE_RX_SINGLE 0x06
#define OPMODE_CAD       0x07

// ----------------------------------------
// FSK opmode bits
// bits 6:5 are the same for 1272 and 1276
#define OPMODE_FSK_SX127x_ModulationType_FSK            (0u << 5)
#define OPMODE_FSK_SX127x_ModulationType_OOK            (1u << 5)
#define OPMODE_FSK_SX127x_ModulationType_MASK           (3u << 5)

// bits 4:3 are different for 1272
#define OPMODE_FSK_SX1272_ModulationShaping_FSK_None    (0u << 3)
#define OPMODE_FSK_SX1272_ModulationShaping_FSK_BT1_0   (1u << 3)
#define OPMODE_FSK_SX1272_ModulationShaping_FSK_BT0_5   (2u << 3)
#define OPMODE_FSK_SX1272_ModulationShaping_FSK_BT0_3   (3u << 3)
#define OPMODE_FSK_SX1272_ModulationShaping_OOK_None    (0u << 3)
#define OPMODE_FSK_SX1272_ModulationShaping_OOK_BR      (1u << 3)
#define OPMODE_FSK_SX1272_ModulationShaping_OOK_2BR     (2u << 3)

#define OPMODE_FSK_SX1272_ModulationShaping_MASK        (3u << 3)

// SX1276
#define OPMODE_FSK_SX1276_LowFrequencyModeOn            (1u << 3)

// define the opmode bits apporpriate for the 127x in use.
#if defined(CFG_sx1272_radio)
# define    OPMODE_FSK_SX127X_SETUP     (OPMODE_FSK_SX127x_ModulationType_FSK | \
                                         OPMODE_FSK_SX1272_ModulationShaping_FSK_BT0_5)
#elif defined(CFG_sx1276_radio)
# define    OPMODE_FSK_SX127X_SETUP     (OPMODE_FSK_SX127x_ModulationType_FSK)
#endif

// ----------------------------------------
// LoRa opmode bits
#define OPMODE_LORA_SX127x_AccessSharedReg              (1u << 6)
#define OPMODE_LORA_SX1276_LowFrequencyModeOn           (1u << 3)

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK 0x80
#define IRQ_LORA_RXDONE_MASK 0x40
#define IRQ_LORA_CRCERR_MASK 0x20
#define IRQ_LORA_HEADER_MASK 0x10
#define IRQ_LORA_TXDONE_MASK 0x08
#define IRQ_LORA_CDDONE_MASK 0x04
#define IRQ_LORA_FHSSCH_MASK 0x02
#define IRQ_LORA_CDDETD_MASK 0x01

#define IRQ_FSK1_MODEREADY_MASK         0x80
#define IRQ_FSK1_RXREADY_MASK           0x40
#define IRQ_FSK1_TXREADY_MASK           0x20
#define IRQ_FSK1_PLLLOCK_MASK           0x10
#define IRQ_FSK1_RSSI_MASK              0x08
#define IRQ_FSK1_TIMEOUT_MASK           0x04
#define IRQ_FSK1_PREAMBLEDETECT_MASK    0x02
#define IRQ_FSK1_SYNCADDRESSMATCH_MASK  0x01
#define IRQ_FSK2_FIFOFULL_MASK          0x80
#define IRQ_FSK2_FIFOEMPTY_MASK         0x40
#define IRQ_FSK2_FIFOLEVEL_MASK         0x20
#define IRQ_FSK2_FIFOOVERRUN_MASK       0x10
#define IRQ_FSK2_PACKETSENT_MASK        0x08
#define IRQ_FSK2_PAYLOADREADY_MASK      0x04
#define IRQ_FSK2_CRCOK_MASK             0x02
#define IRQ_FSK2_LOWBAT_MASK            0x01

// ----------------------------------------
// DIO function mappings                D0D1D2D3
#define MAP_DIO0_LORA_RXDONE   0x00  // 00------
#define MAP_DIO0_LORA_TXDONE   0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT   0x00  // --00----
#define MAP_DIO1_LORA_NOP      0x30  // --11----
#define MAP_DIO2_LORA_NOP      0x0C  // ----11--

#define MAP_DIO0_FSK_READY     0x00  // 00------ (packet sent / payload ready)
#define MAP_DIO1_FSK_NOP       0x30  // --11----
#define MAP_DIO2_FSK_TXNOP     0x04  // ----01--
#define MAP_DIO2_FSK_TIMEOUT   0x08  // ----10--


// FSK IMAGECAL defines
#define RF_IMAGECAL_AUTOIMAGECAL_MASK               0x7F
#define RF_IMAGECAL_AUTOIMAGECAL_ON                 0x80
#define RF_IMAGECAL_AUTOIMAGECAL_OFF                0x00  // Default

#define RF_IMAGECAL_IMAGECAL_MASK                   0xBF
#define RF_IMAGECAL_IMAGECAL_START                  0x40

#define RF_IMAGECAL_IMAGECAL_RUNNING                0x20
#define RF_IMAGECAL_IMAGECAL_DONE                   0x00  // Default

// LNA gain constant. Bits 4..0 have different meaning for 1272 and 1276, but
// by chance, the bit patterns we use are the same.
#ifdef CFG_sx1276_radio
#define LNA_RX_GAIN (0x20|0x3)
#elif CFG_sx1272_radio
#define LNA_RX_GAIN (0x20|0x03)
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif

// RADIO STATE
// (initialized by radio_init(), used by radio_rand1())
static u1_t randbuf[16];


static void writeReg (u1_t addr, u1_t data ) {
    hal_spi_write(addr | 0x80, &data, 1);
}

static u1_t readReg (u1_t addr) {
    u1_t buf[1];
    hal_spi_read(addr & 0x7f, buf, 1);
    return buf[0];
}

static void writeBuf (u1_t addr, xref2u1_t buf, u1_t len) {
    hal_spi_write(addr | 0x80, buf, len);
}

static void readBuf (u1_t addr, xref2u1_t buf, u1_t len) {
    hal_spi_read(addr & 0x7f, buf, len);
}

static void requestModuleActive(bit_t state) {
    ostime_t const ticks = hal_setModuleActive(state);

    if (ticks)
        hal_waitUntil(os_getTime() + ticks);;
}

static void writeOpmode(u1_t mode) {
    u1_t const maskedMode = mode & OPMODE_MASK;
    if (maskedMode != OPMODE_SLEEP)
        requestModuleActive(1);
    writeReg(RegOpMode, mode);
    if (maskedMode == OPMODE_SLEEP)
        requestModuleActive(0);
}

static void opmode (u1_t mode) {
    writeOpmode((readReg(RegOpMode) & ~OPMODE_MASK) | mode);
}

static void opmodeLora() {
    u1_t u = OPMODE_LORA;
#ifdef CFG_sx1276_radio
    if (LMIC.freq <= SX127X_FREQ_LF_MAX) {
        u |= OPMODE_FSK_SX1276_LowFrequencyModeOn;
    }
#endif
    writeOpmode(u);
}

static void opmodeFSK() {
    u1_t u = OPMODE_FSK_SX127X_SETUP;

#ifdef CFG_sx1276_radio
    if (LMIC.freq <= SX127X_FREQ_LF_MAX) {
        u |= OPMODE_FSK_SX1276_LowFrequencyModeOn;
    }
#endif
    writeOpmode(u);
}

// configure LoRa modem (cfg1, cfg2)
static void configLoraModem () {
    sf_t sf = getSf(LMIC.rps);

#ifdef CFG_sx1276_radio
        u1_t mc1 = 0, mc2 = 0, mc3 = 0;

        bw_t const bw = getBw(LMIC.rps);

        switch (bw) {
        case BW125: mc1 |= SX1276_MC1_BW_125; break;
        case BW250: mc1 |= SX1276_MC1_BW_250; break;
        case BW500: mc1 |= SX1276_MC1_BW_500; break;
        default:
            ASSERT(0);
        }
        switch( getCr(LMIC.rps) ) {
        case CR_4_5: mc1 |= SX1276_MC1_CR_4_5; break;
        case CR_4_6: mc1 |= SX1276_MC1_CR_4_6; break;
        case CR_4_7: mc1 |= SX1276_MC1_CR_4_7; break;
        case CR_4_8: mc1 |= SX1276_MC1_CR_4_8; break;
        default:
            ASSERT(0);
        }

        if (getIh(LMIC.rps)) {
            mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
            writeReg(LORARegPayloadLength, getIh(LMIC.rps)); // required length
        }
        // set ModemConfig1
        writeReg(LORARegModemConfig1, mc1);

        mc2 = (SX1272_MC2_SF7 + ((sf-1)<<4) + ((LMIC.rxsyms >> 8) & 0x3) );
        if (getNocrc(LMIC.rps) == 0) {
            mc2 |= SX1276_MC2_RX_PAYLOAD_CRCON;
        }
#if CFG_TxContinuousMode
        // Only for testing
        // set ModemConfig2 (sf, TxContinuousMode=1, AgcAutoOn=1 SymbTimeoutHi=00)
        mc2 |= 0x8;
#endif
        writeReg(LORARegModemConfig2, mc2);

        mc3 = SX1276_MC3_AGCAUTO;

        if ( ((sf == SF11 || sf == SF12) && bw == BW125) ||
             ((sf == SF12) && bw == BW250) ) {
            mc3 |= SX1276_MC3_LOW_DATA_RATE_OPTIMIZE;
        }
        writeReg(LORARegModemConfig3, mc3);

        // Errata 2.1: Sensitivity optimization with 500 kHz bandwidth
        u1_t rHighBwOptimize1;
        u1_t rHighBwOptimize2;

        rHighBwOptimize1 = 0x03;
        rHighBwOptimize2 = 0;

        if (bw == BW500) {
            if (LMIC.freq > SX127X_FREQ_LF_MAX) {
                rHighBwOptimize1 = 0x02;
                rHighBwOptimize2 = 0x64;
            } else {
                rHighBwOptimize1 = 0x02;
                rHighBwOptimize2 = 0x7F;
            }
        }

        writeReg(LORARegHighBwOptimize1, rHighBwOptimize1);
        if (rHighBwOptimize2 != 0)
            writeReg(LORARegHighBwOptimize2, rHighBwOptimize2);

#elif CFG_sx1272_radio
        u1_t mc1 = (getBw(LMIC.rps)<<6);

        switch( getCr(LMIC.rps) ) {
        case CR_4_5: mc1 |= SX1272_MC1_CR_4_5; break;
        case CR_4_6: mc1 |= SX1272_MC1_CR_4_6; break;
        case CR_4_7: mc1 |= SX1272_MC1_CR_4_7; break;
        case CR_4_8: mc1 |= SX1272_MC1_CR_4_8; break;
        }

        if ((sf == SF11 || sf == SF12) && getBw(LMIC.rps) == BW125) {
            mc1 |= SX1272_MC1_LOW_DATA_RATE_OPTIMIZE;
        }

        if (getNocrc(LMIC.rps) == 0) {
            mc1 |= SX1272_MC1_RX_PAYLOAD_CRCON;
        }

        if (getIh(LMIC.rps)) {
            mc1 |= SX1272_MC1_IMPLICIT_HEADER_MODE_ON;
            writeReg(LORARegPayloadLength, getIh(LMIC.rps)); // required length
        }
        // set ModemConfig1
        writeReg(LORARegModemConfig1, mc1);

        // set ModemConfig2 (sf, AgcAutoOn=1 SymbTimeoutHi)
        u1_t mc2;
        mc2 = (SX1272_MC2_SF7 + ((sf-1)<<4)) | 0x04 | ((LMIC.rxsyms >> 8) & 0x3);

#if CFG_TxContinuousMode
        // Only for testing
        // set ModemConfig2 (sf, TxContinuousMode=1, AgcAutoOn=1 SymbTimeoutHi=00)
        mc2 |= 0x8;
#endif

        writeReg(LORARegModemConfig2, mc2);

#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

static void configChannel () {
    // set frequency: FQ = (FRF * 32 Mhz) / (2 ^ 19)
    uint64_t frf = ((uint64_t)LMIC.freq << 19) / 32000000;
    writeReg(RegFrfMsb, (u1_t)(frf>>16));
    writeReg(RegFrfMid, (u1_t)(frf>> 8));
    writeReg(RegFrfLsb, (u1_t)(frf>> 0));
}

// On the SX1276, we have several possible configs.
// 1) using RFO, MaxPower==0: in that case power is -4 to 11 dBm
// 2) using RFO, MaxPower==7: in that case, power is 0 to 14 dBm
//      (can't select 15 dBm).
//	note we can use -4..11 w/o Max and then 12..14 w/Max, and
//	we really don't need to ask anybody.
// 3) using PA_BOOST, PaDac = 4: in that case power range is 2 to 17 dBm;
//	use this for 15..17 if authorized.
// 4) using PA_BOOST, PaDac = 7, OutputPower=0xF: in that case, power is 20 dBm
//		(and perhaps 0xE is 19, 0xD is 18 dBm, but datasheet isn't clear.)
//    and duty cycle must be <= 1%.
//
// In addition, there are some boards for which PA_BOOST can only be used if the
// channel frequency is greater than SX127X_FREQ_LF_MAX.
//
// The SX1272 is similar but has no MaxPower bit:
// 1) using RFO: power is -1 to 13 dBm (datasheet implies max OutputPower value is 14 for 13 dBm)
// 2) using PA_BOOST, PaDac = 0x84: power is 2 to 17 dBm;
//	use this for 14..17 if authorized
// 3) using PA_BOOST, PaDac = 0x87, OutputPower = 0xF: power is 20dBm
//    and duty cycle must be <= 1%
//
// The general policy is to use the lowest power variant that will get us where we
// need to be.
//

static void configPower () {
    // our input paramter -- might be different than LMIC.txpow!
    s1_t const req_pw = (s1_t)LMIC.radio_txpow;
    // the effective power
    s1_t eff_pw;
    // the policy; we're going to compute this.
    u1_t policy;
    // what we'll write to RegPaConfig
    u1_t rPaConfig;
    // what we'll write to RegPaDac
    u1_t rPaDac;
    // what we'll write to RegOcp
    u1_t rOcp;

#ifdef CFG_sx1276_radio
    if (req_pw >= 20) {
        policy = LMICHAL_radio_tx_power_policy_20dBm;
        eff_pw = 20;
    } else if (req_pw >= 14) {
        policy = LMICHAL_radio_tx_power_policy_paboost;
        if (req_pw > 17) {
            eff_pw = 17;
        } else {
            eff_pw = req_pw;
        }
    } else {
        policy = LMICHAL_radio_tx_power_policy_rfo;
        if (req_pw < -4) {
            eff_pw = -4;
        } else {
            eff_pw = req_pw;
        }
    }

    policy = hal_getTxPowerPolicy(policy, eff_pw, LMIC.freq);

    switch (policy) {
    default:
    case LMICHAL_radio_tx_power_policy_rfo:
        rPaDac = SX127X_PADAC_POWER_NORMAL;
        rOcp = SX127X_OCP_MAtoBITS(80);

        if (eff_pw > 14)
            eff_pw = 14;
        if (eff_pw > 11) {
            // some Semtech code uses this down to eff_pw == 0.
            rPaConfig = eff_pw | SX1276_PAC_MAX_POWER_MASK;
        } else {
            if (eff_pw < -4)
                eff_pw = -4;
            rPaConfig = eff_pw + 4;
        }
        break;

    // some radios (HopeRF RFM95W) don't support RFO well,
    // so the policy might *raise* rfo to paboost. That means
    // we have to re-check eff_pw, which might be too small.
    // (And, of course, it might also be too large.)
    case LMICHAL_radio_tx_power_policy_paboost:
        // It seems that SX127x doesn't like eff_pw 10 when in FSK mode.
        if (getSf(LMIC.rps) == FSK && eff_pw < 11) {
            eff_pw = 11;
        }
        rPaDac = SX127X_PADAC_POWER_NORMAL;
        rOcp = SX127X_OCP_MAtoBITS(100);
        if (eff_pw > 17)
            eff_pw = 17;
        else if (eff_pw < 2)
            eff_pw = 2;
        rPaConfig = (eff_pw - 2) | SX1276_PAC_PA_SELECT_PA_BOOST;
        break;

    case LMICHAL_radio_tx_power_policy_20dBm:
        rPaDac = SX127X_PADAC_POWER_20dBm;
        rOcp = SX127X_OCP_MAtoBITS(130);
        rPaConfig = 0xF | SX1276_PAC_PA_SELECT_PA_BOOST;
        break;
    }

#elif CFG_sx1272_radio
    if (req_pw >= 20) {
        policy = LMICHAL_radio_tx_power_policy_20dBm;
            eff_pw = 20;
    } else if (eff_pw >= 14) {
        policy = LMICHAL_radio_tx_power_policy_paboost;
        if (eff_pw > 17) {
            eff_pw = 17;
        } else {
            eff_pw = req_pw;
        }
    } else {
        policy = LMICHAL_radio_tx_power_policy_rfo;
        if (req_pw < -1) {
            eff_pw = -1;
        } else {
            eff_pw = req_pw;
        }
    }

    policy = hal_getTxPowerPolicy(policy, eff_pw, LMIC.freq);

    switch (policy) {
    default:
    case LMICHAL_radio_tx_power_policy_rfo:
        rPaDac = SX127X_PADAC_POWER_NORMAL;
        rOcp = SX127X_OCP_MAtoBITS(50);

        if (eff_pw > 13)
            eff_pw = 13;

        rPaConfig = eff_pw + 1;
        break;

    case LMICHAL_radio_tx_power_policy_paboost:
        rPaDac = SX127X_PADAC_POWER_NORMAL;
        rOcp = SX127X_OCP_MAtoBITS(100);

        if (eff_pw > 17)
            eff_pw = 17;

        rPaConfig = (eff_pw - 2) | SX1272_PAC_PA_SELECT_PA_BOOST;
        break;

    case LMICHAL_radio_tx_power_policy_20dBm:
        rPaDac = SX127X_PADAC_POWER_20dBm;
        rOcp = SX127X_OCP_MAtoBITS(130);

        rPaConfig = 0xF | SX1276_PAC_PA_SELECT_PA_BOOST;
        break;
    }
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */

    writeReg(RegPaConfig, rPaConfig);
    writeReg(RegPaDac, (readReg(RegPaDac) & ~SX127X_PADAC_POWER_MASK) | rPaDac);
    writeReg(RegOcp, rOcp | SX127X_OCP_ENA);
}

static void setupFskRxTx(bit_t fDisableAutoClear) {
    // set bitrate
    writeReg(FSKRegBitrateMsb, 0x02); // 50kbps
    writeReg(FSKRegBitrateLsb, 0x80);
    // set frequency deviation
    writeReg(FSKRegFdevMsb, 0x01); // +/- 25kHz
    writeReg(FSKRegFdevLsb, 0x99);

    // set sync config
    writeReg(FSKRegSyncConfig, 0x12); // no auto restart, preamble 0xAA, enable, fill FIFO, 3 bytes sync

    // set packet config
    writeReg(FSKRegPacketConfig1, fDisableAutoClear ? 0xD8 : 0xD0); // var-length, whitening, crc, no auto-clear, no adr filter
    writeReg(FSKRegPacketConfig2, 0x40); // packet mode

    // set sync value
    writeReg(FSKRegSyncValue1, 0xC1);
    writeReg(FSKRegSyncValue2, 0x94);
    writeReg(FSKRegSyncValue3, 0xC1);
}

static void txfsk () {
    // select FSK modem (from sleep mode)
    opmodeFSK();

    // enter standby mode (required for FIFO loading))
    opmode(OPMODE_STANDBY);
    // set bitrate etc
    setupFskRxTx(/* don't autoclear CRC */ 0);

    // frame and packet handler settings
    writeReg(FSKRegPreambleMsb, 0x00);
    writeReg(FSKRegPreambleLsb, 0x05);

    // configure frequency
    configChannel();
    // configure output power
    configPower();

#ifdef CFG_sx1276_radio
    // select Gausian filter BT=0.5, default ramp.
    writeReg(RegPaRamp, 0x29);
#endif

    // set the IRQ mapping DIO0=PacketSent DIO1=NOP DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_FSK_READY|MAP_DIO1_FSK_NOP|MAP_DIO2_FSK_TXNOP);

    // initialize the payload size and address pointers
    // TODO(tmm@mcci.com): datasheet says this is not used in variable packet length mode
    writeReg(FSKRegPayloadLength, LMIC.dataLen+1); // (insert length byte into payload))

    // download length byte and buffer to the radio FIFO
    writeReg(RegFifo, LMIC.dataLen);
    writeBuf(RegFifo, LMIC.frame, LMIC.dataLen);

    // enable antenna switch for TX
    hal_pin_rxtx(1);

    // now we actually start the transmission
    if (LMIC.txend) {
        u4_t nLate = hal_waitUntil(LMIC.txend); // busy wait until exact tx time
        if (nLate > 0) {
            LMIC.radio.txlate_ticks += nLate;
            ++LMIC.radio.txlate_count;
        }
    }
    LMICOS_logEventUint32("+Tx FSK", LMIC.dataLen);
    opmode(OPMODE_TX);
}

static void txlora () {
    // select LoRa modem (from sleep mode)
    //writeReg(RegOpMode, OPMODE_LORA);
    opmodeLora();
    ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);

    // enter standby mode (required for FIFO loading))
    opmode(OPMODE_STANDBY);
    // configure LoRa modem (cfg1, cfg2)
    configLoraModem();
    // configure frequency
    configChannel();
    // configure output power
#ifdef CFG_sx1272_radio
    writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
#elif defined(CFG_sx1276_radio)
    writeReg(RegPaRamp, 0x08);     // set PA ramp-up time 50 uSec, clear FSK bits
#endif
    configPower();
    // set sync word
    writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);

    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    // mask all IRQs but TxDone
    writeReg(LORARegIrqFlagsMask, ~IRQ_LORA_TXDONE_MASK);

    // initialize the payload size and address pointers
    writeReg(LORARegFifoTxBaseAddr, 0x00);
    writeReg(LORARegFifoAddrPtr, 0x00);
    writeReg(LORARegPayloadLength, LMIC.dataLen);

    // download buffer to the radio FIFO
    writeBuf(RegFifo, LMIC.frame, LMIC.dataLen);

    // enable antenna switch for TX
    hal_pin_rxtx(1);

    // now we actually start the transmission
    if (LMIC.txend) {
        u4_t nLate = hal_waitUntil(LMIC.txend); // busy wait until exact tx time
        if (nLate) {
            LMIC.radio.txlate_ticks += nLate;
            ++LMIC.radio.txlate_count;
        }
    }
    LMICOS_logEventUint32("+Tx LoRa", LMIC.dataLen);
    opmode(OPMODE_TX);

#if LMIC_DEBUG_LEVEL > 0
    u1_t sf = getSf(LMIC.rps) + 6; // 1 == SF7
    u1_t bw = getBw(LMIC.rps);
    u1_t cr = getCr(LMIC.rps);
    LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": TXMODE, freq=%"PRIu32", len=%d, SF=%d, BW=%d, CR=4/%d, IH=%d\n",
           os_getTime(), LMIC.freq, LMIC.dataLen, sf,
           bw == BW125 ? 125 : (bw == BW250 ? 250 : 500),
           cr == CR_4_5 ? 5 : (cr == CR_4_6 ? 6 : (cr == CR_4_7 ? 7 : 8)),
           getIh(LMIC.rps)
   );
#endif
}

// start transmitter (buf=LMIC.frame, len=LMIC.dataLen)
static void starttx () {
    u1_t const rOpMode = readReg(RegOpMode);

    // originally, this code ASSERT()ed, but asserts are both bad and
    // blunt instruments. If we see that we're not in sleep mode,
    // force sleep (because we might have to switch modes)
    if ((rOpMode & OPMODE_MASK) != OPMODE_SLEEP) {
#if LMIC_DEBUG_LEVEL > 0
        LMIC_DEBUG_PRINTF("?%s: OPMODE != OPMODE_SLEEP: %#02x\n", __func__, rOpMode);
#endif
        opmode(OPMODE_SLEEP);
        hal_waitUntil(os_getTime() + ms2osticks(1));
    }

    if (LMIC.lbt_ticks > 0) {
        oslmic_radio_rssi_t rssi;
        radio_monitor_rssi(LMIC.lbt_ticks, &rssi);
#if LMIC_X_DEBUG_LEVEL > 0
        LMIC_X_DEBUG_PRINTF("LBT rssi max:min=%d:%d %d times in %d\n", rssi.max_rssi, rssi.min_rssi, rssi.n_rssi, LMIC.lbt_ticks);
#endif

        if (rssi.max_rssi >= LMIC.lbt_dbmax) {
            // complete the request by scheduling the job
            os_setCallback(&LMIC.osjob, LMIC.osjob.func);
            return;
        }
    }

    if(getSf(LMIC.rps) == FSK) { // FSK modem
        txfsk();
    } else { // LoRa modem
        txlora();
    }
    // the radio will go back to STANDBY mode as soon as the TX is finished
    // the corresponding IRQ will inform us about completion.
}

enum { RXMODE_SINGLE, RXMODE_SCAN, RXMODE_RSSI };

static CONST_TABLE(u1_t, rxlorairqmask)[] = {
    [RXMODE_SINGLE] = IRQ_LORA_RXDONE_MASK|IRQ_LORA_RXTOUT_MASK,
    [RXMODE_SCAN]   = IRQ_LORA_RXDONE_MASK,
    [RXMODE_RSSI]   = 0x00,
};

//! \brief handle late RX events.
//! \param nLate is the number of `ostime_t` ticks that the event was late.
//! \details If nLate is non-zero, increment the count of events, totalize
//! the number of ticks late, and (if implemented) adjust the estimate of
//! what would be best to return from `os_getRadioRxRampup()`.
static void rxlate (u4_t nLate) {
    if (nLate) {
            LMIC.radio.rxlate_ticks += nLate;
            ++LMIC.radio.rxlate_count;
    }
}

// start LoRa receiver (time=LMIC.rxtime, timeout=LMIC.rxsyms, result=LMIC.frame[LMIC.dataLen])
static void rxlora (u1_t rxmode) {
    // select LoRa modem (from sleep mode)
    opmodeLora();
    ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);
    // enter standby mode (warm up))
    opmode(OPMODE_STANDBY);
    // don't use MAC settings at startup
    if(rxmode == RXMODE_RSSI) { // use fixed settings for rssi scan
        writeReg(LORARegModemConfig1, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1);
        writeReg(LORARegModemConfig2, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2);
    } else { // single or continuous rx mode
        // configure LoRa modem (cfg1, cfg2)
        configLoraModem();
        // configure frequency
        configChannel();
    }
    // set LNA gain
    writeReg(RegLna, LNA_RX_GAIN);
    // set max payload size
    writeReg(LORARegPayloadMaxLength, MAX_LEN_FRAME);
#if !defined(DISABLE_INVERT_IQ_ON_RX) /* DEPRECATED(tmm@mcci.com); #250. remove test, always include code in V3 */
    // use inverted I/Q signal (prevent mote-to-mote communication)

    // XXX: use flag to switch on/off inversion
    if (LMIC.noRXIQinversion) {
        writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ) & ~(1<<6));
    } else {
        writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ)|(1<<6));
    }
#endif

    // Errata 2.3 - receiver spurious reception of a LoRa signal
    bw_t const bw = getBw(LMIC.rps);
    u1_t const rDetectOptimize = (readReg(LORARegDetectOptimize) & 0x78) | 0x03;
    if (bw < BW500) {
        writeReg(LORARegDetectOptimize, rDetectOptimize);
        writeReg(LORARegIffReq1, 0x40);
        writeReg(LORARegIffReq2, 0x40);
    } else {
        writeReg(LORARegDetectOptimize, rDetectOptimize | 0x80);
    }

    // set symbol timeout (for single rx)
    writeReg(LORARegSymbTimeoutLsb, (uint8_t) LMIC.rxsyms);
    // set sync word
    writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);

    // configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_RXDONE|MAP_DIO1_LORA_RXTOUT|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    // enable required radio IRQs
    writeReg(LORARegIrqFlagsMask, ~TABLE_GET_U1(rxlorairqmask, rxmode));

    // enable antenna switch for RX
    hal_pin_rxtx(0);

    writeReg(LORARegFifoAddrPtr, 0);
    writeReg(LORARegFifoRxBaseAddr, 0);

    // now instruct the radio to receive
    if (rxmode == RXMODE_SINGLE) { // single rx
        u4_t nLate = hal_waitUntil(LMIC.rxtime); // busy wait until exact rx time
        opmode(OPMODE_RX_SINGLE);
        LMICOS_logEventUint32("+Rx LoRa Single", nLate);
        rxlate(nLate);
#if LMIC_DEBUG_LEVEL > 0
        ostime_t now = os_getTime();
        LMIC_DEBUG_PRINTF("start single rx: now-rxtime: %"LMIC_PRId_ostime_t"\n", now - LMIC.rxtime);
#endif
    } else { // continous rx (scan or rssi)
        LMICOS_logEventUint32("+Rx LoRa Continuous", rxmode);
        opmode(OPMODE_RX);
    }

#if LMIC_DEBUG_LEVEL > 0
    if (rxmode == RXMODE_RSSI) {
        LMIC_DEBUG_PRINTF("RXMODE_RSSI\n");
    } else {
        u1_t sf = getSf(LMIC.rps) + 6; // 1 == SF7
        u1_t bw = getBw(LMIC.rps);
        u1_t cr = getCr(LMIC.rps);
        LMIC_DEBUG_PRINTF("%"LMIC_PRId_ostime_t": %s, freq=%"PRIu32", SF=%d, BW=%d, CR=4/%d, IH=%d\n",
               os_getTime(),
               rxmode == RXMODE_SINGLE ? "RXMODE_SINGLE" : (rxmode == RXMODE_SCAN ? "RXMODE_SCAN" : "UNKNOWN_RX"),
               LMIC.freq, sf,
               bw == BW125 ? 125 : (bw == BW250 ? 250 : 500),
               cr == CR_4_5 ? 5 : (cr == CR_4_6 ? 6 : (cr == CR_4_7 ? 7 : 8)),
               getIh(LMIC.rps)
       );
    }
#endif
}

static void rxfsk (u1_t rxmode) {
    // only single or continuous rx (no noise sampling)
    if (rxmode == RXMODE_SCAN) {
        // indicate no bytes received.
        LMIC.dataLen = 0;
        // complete the request by scheduling the job.
        os_setCallback(&LMIC.osjob, LMIC.osjob.func);
    }

    // select FSK modem (from sleep mode)
    //writeReg(RegOpMode, 0x00); // (not LoRa)
    opmodeFSK();
    ASSERT((readReg(RegOpMode) & OPMODE_LORA) == 0);
    // enter standby mode (warm up))
    opmode(OPMODE_STANDBY);
    // configure frequency
    configChannel();
    // set LNA gain
    writeReg(RegLna, LNA_RX_GAIN);  // max gain, boost enable.
    // configure receiver
    writeReg(FSKRegRxConfig, 0x1E); // AFC auto, AGC, trigger on preamble?!?
    // set receiver bandwidth
    writeReg(FSKRegRxBw, 0x0B);     // 50kHz SSb
    // set AFC bandwidth
    writeReg(FSKRegAfcBw, 0x12);    // 83.3kHz SSB
    // set preamble detection
    writeReg(FSKRegPreambleDetect, 0xAA); // enable, 2 bytes, 10 chip errors
    // set preamble timeout
    writeReg(FSKRegRxTimeout2, 0xFF);//(LMIC.rxsyms+1)/2);
    // set bitrate, autoclear CRC
    setupFskRxTx(1);

    // configure DIO mapping DIO0=PayloadReady DIO1=NOP DIO2=TimeOut
    writeReg(RegDioMapping1, MAP_DIO0_FSK_READY|MAP_DIO1_FSK_NOP|MAP_DIO2_FSK_TIMEOUT);

    // enable antenna switch for RX
    hal_pin_rxtx(0);

    // now instruct the radio to receive
    if (rxmode == RXMODE_SINGLE) {
        u4_t nLate = hal_waitUntil(LMIC.rxtime); // busy wait until exact rx time
        opmode(OPMODE_RX); // no single rx mode available in FSK
        LMICOS_logEventUint32("+Rx FSK", nLate);
        rxlate(nLate);
    } else {
        LMICOS_logEvent("+Rx FSK Continuous");
        opmode(OPMODE_RX);
    }
}

static void startrx (u1_t rxmode) {
    ASSERT( (readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP );
    if(getSf(LMIC.rps) == FSK) { // FSK modem
        rxfsk(rxmode);
    } else { // LoRa modem
        rxlora(rxmode);
    }
    // the radio will go back to STANDBY mode as soon as the RX is finished
    // or timed out, and the corresponding IRQ will inform us about completion.
}

//! \brief Initialize radio at system startup.
//!
//! \details This procedure is called during initialization by the `os_init()`
//! routine. It does a hardware reset of the radio, checks the version and confirms
//! that we're operating a suitable chip, and gets a random seed from wideband
//! noise rssi. It then puts the radio to sleep.
//!
//! \result True if successful, false if it doesn't look like the right radio is attached.
//!
//! \pre
//! Preconditions must be observed, or you'll get hangs during initialization.
//!
//! - The `hal_pin_..()` functions must be ready for use.
//! - The `hal_waitUntl()` function must be ready for use. This may mean that interrupts
//!   are enabled.
//! - The `hal_spi_..()` functions must be ready for use.
//!
//! Generally, all these are satisfied by a call to `hal_init_with_pinmap()`.
//!
int radio_init () {
    requestModuleActive(1);

    // manually reset radio
#ifdef CFG_sx1276_radio
    hal_pin_rst(0); // drive RST pin low
#else
    hal_pin_rst(1); // drive RST pin high
#endif
    hal_waitUntil(os_getTime()+ms2osticks(1)); // wait >100us
    hal_pin_rst(2); // configure RST pin floating!
    hal_waitUntil(os_getTime()+ms2osticks(5)); // wait 5ms

    opmode(OPMODE_SLEEP);

    // some sanity checks, e.g., read version number
    u1_t v = readReg(RegVersion);
#ifdef CFG_sx1276_radio
    if(v != 0x12 )
        return 0;
#elif CFG_sx1272_radio
    if(v != 0x22)
        return 0;
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif
    // set the tcxo input, if needed
    if (hal_queryUsingTcxo())
        writeReg(RegTcxo, readReg(RegTcxo) | RegTcxo_TcxoInputOn);

    // seed 15-byte randomness via noise rssi
    rxlora(RXMODE_RSSI);
    while( (readReg(RegOpMode) & OPMODE_MASK) != OPMODE_RX ); // continuous rx
    for(int i=1; i<16; i++) {
        for(int j=0; j<8; j++) {
            u1_t b; // wait for two non-identical subsequent least-significant bits
            while( (b = readReg(LORARegRssiWideband) & 0x01) == (readReg(LORARegRssiWideband) & 0x01) );
            randbuf[i] = (randbuf[i] << 1) | b;
        }
    }
    randbuf[0] = 16; // set initial index

#ifdef CFG_sx1276mb1_board
    // chain calibration
    writeReg(RegPaConfig, 0);

    // Launch Rx chain calibration for LF band
    writeReg(FSKRegImageCal, (readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_MASK)|RF_IMAGECAL_IMAGECAL_START);
    while((readReg(FSKRegImageCal)&RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING){ ; }

    // Sets a Frequency in HF band
    u4_t frf = 868000000;
    writeReg(RegFrfMsb, (u1_t)(frf>>16));
    writeReg(RegFrfMid, (u1_t)(frf>> 8));
    writeReg(RegFrfLsb, (u1_t)(frf>> 0));

    // Launch Rx chain calibration for HF band
    writeReg(FSKRegImageCal, (readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_MASK)|RF_IMAGECAL_IMAGECAL_START);
    while((readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) { ; }
#endif /* CFG_sx1276mb1_board */

    opmode(OPMODE_SLEEP);

    return 1;
}

// return next random byte derived from seed buffer
// (buf[0] holds index of next byte to be returned)
u1_t radio_rand1 () {
    u1_t i = randbuf[0];
    ASSERT( i != 0 );
    if( i==16 ) {
        os_aes(AES_ENC, randbuf, 16); // encrypt seed with any key
        i = 0;
    }
    u1_t v = randbuf[i++];
    randbuf[0] = i;
    return v;
}

u1_t radio_rssi () {
    u1_t r = readReg(LORARegRssiValue);
    return r;
}

/// \brief get the current RSSI on the current channel.
///
/// monitor rssi for specified number of ostime_t ticks, and return statistics
/// This puts the radio into RX continuous mode, waits long enough for the
/// oscillators to start and the PLL to lock, and then measures for the specified
/// period of time.  The radio is then returned to idle.
///
/// RSSI returned is expressed in units of dB, and is offset according to the
/// current radio setting per section 5.5.5 of Semtech 1276 datasheet.
///
/// \param nTicks How long to monitor
/// \param pRssi pointer to structure to fill in with RSSI data.
///
void radio_monitor_rssi(ostime_t nTicks, oslmic_radio_rssi_t *pRssi) {
    uint8_t rssiMax, rssiMin;
    uint16_t rssiSum;
    uint16_t rssiN;

    int rssiAdjust;
    ostime_t tBegin;
    int notDone;

    rxlora(RXMODE_SCAN);

    // while we're waiting for the PLLs to spin up, determine which
    // band we're in and choose the base RSSI.
#if defined(CFG_sx1276_radio)
    if (LMIC.freq > SX127X_FREQ_LF_MAX) {
            rssiAdjust = SX1276_RSSI_ADJUST_HF;
    } else {
            rssiAdjust = SX1276_RSSI_ADJUST_LF;
    }
#elif defined(CFG_sx1272_radio)
    rssiAdjust = SX1272_RSSI_ADJUST;
#endif
    rssiAdjust += hal_getRssiCal();

    // zero the results
    rssiMax = 255;
    rssiMin = 0;
    rssiSum = 0;
    rssiN = 0;

    // wait for PLLs
    hal_waitUntil(os_getTime() + SX127X_RX_POWER_UP);

    // scan for the desired time.
    tBegin = os_getTime();
    rssiMax = 0;

    /* Per bug report from tanupoo, it's critical that interrupts be enabled
     * in the loop below so that `os_getTime()` always advances.
     */
    do {
        ostime_t now;

        u1_t rssiNow = readReg(LORARegRssiValue);

        if (rssiMax < rssiNow)
                rssiMax = rssiNow;
        if (rssiNow < rssiMin)
                rssiMin = rssiNow;
        rssiSum += rssiNow;
        ++rssiN;
        now = os_getTime();
        notDone = now - (tBegin + nTicks) < 0;
    } while (notDone);

    // put radio back to sleep
    opmode(OPMODE_SLEEP);

    // compute the results
    pRssi->max_rssi = (s2_t) (rssiMax + rssiAdjust);
    pRssi->min_rssi = (s2_t) (rssiMin + rssiAdjust);
    pRssi->mean_rssi = (s2_t) (rssiAdjust + ((rssiSum + (rssiN >> 1)) / rssiN));
    pRssi->n_rssi = rssiN;
}

static CONST_TABLE(u2_t, LORA_RXDONE_FIXUP)[] = {
    [FSK]  =     us2osticks(0), // (   0 ticks)
    [SF7]  =     us2osticks(0), // (   0 ticks)
    [SF8]  =  us2osticks(1648), // (  54 ticks)
    [SF9]  =  us2osticks(3265), // ( 107 ticks)
    [SF10] =  us2osticks(7049), // ( 231 ticks)
    [SF11] = us2osticks(13641), // ( 447 ticks)
    [SF12] = us2osticks(31189), // (1022 ticks)
};

// called by hal ext IRQ handler
// (radio goes to stanby mode after tx/rx operations)
void radio_irq_handler (u1_t dio) {
    radio_irq_handler_v2(dio, os_getTime());
}

void radio_irq_handler_v2 (u1_t dio, ostime_t now) {
    LMIC_API_PARAMETER(dio);

#if CFG_TxContinuousMode
    // in continuous mode, we don't use the now parameter.
    LMIC_UNREFERENCED_PARAMETER(now);

    // clear radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    u1_t p = readReg(LORARegFifoAddrPtr);
    writeReg(LORARegFifoAddrPtr, 0x00);
    u1_t s = readReg(RegOpMode);
    u1_t c = readReg(LORARegModemConfig2);
    LMICOS_logEventUint32("+Tx LoRa Continuous", (r << 8) + c);
    opmode(OPMODE_TX);
    return;
#else /* ! CFG_TxContinuousMode */

#if LMIC_DEBUG_LEVEL > 0
    ostime_t const entry = now;
#endif
    if( (readReg(RegOpMode) & OPMODE_LORA) != 0) { // LORA modem
        u1_t flags = readReg(LORARegIrqFlags);
        LMIC.saveIrqFlags = flags;
        LMICOS_logEventUint32("radio_irq_handler_v2: LoRa", flags);
        LMIC_X_DEBUG_PRINTF("IRQ=%02x\n", flags);
        if( flags & IRQ_LORA_TXDONE_MASK ) {
            // save exact tx time
            LMIC.txend = now - us2osticks(43); // TXDONE FIXUP
        } else if( flags & IRQ_LORA_RXDONE_MASK ) {
            // save exact rx time
            if(getBw(LMIC.rps) == BW125) {
                now -= TABLE_GET_U2(LORA_RXDONE_FIXUP, getSf(LMIC.rps));
            }
            LMIC.rxtime = now;
            // read the PDU and inform the MAC that we received something
            LMIC.dataLen = (readReg(LORARegModemConfig1) & SX127X_MC1_IMPLICIT_HEADER_MODE_ON) ?
                readReg(LORARegPayloadLength) : readReg(LORARegRxNbBytes);
            // set FIFO read address pointer
            writeReg(LORARegFifoAddrPtr, readReg(LORARegFifoRxCurrentAddr));
            // now read the FIFO
            readBuf(RegFifo, LMIC.frame, LMIC.dataLen);
            // read rx quality parameters
            LMIC.snr  = readReg(LORARegPktSnrValue); // SNR [dB] * 4
            u1_t const rRssi = readReg(LORARegPktRssiValue);
            s2_t rssi = rRssi;
            if (LMIC.freq > SX127X_FREQ_LF_MAX)
                rssi += SX127X_RSSI_ADJUST_HF;
            else
                rssi += SX127X_RSSI_ADJUST_LF;
            if (LMIC.snr < 0)
                rssi = rssi - (-LMIC.snr >> 2);
            else if (rssi > -100) {
                // correct nonlinearity -- this is the same as multiplying rRssi * 16/15 initially.
                rssi += (rRssi / 15);
            }

            LMIC_X_DEBUG_PRINTF("RX snr=%u rssi=%d\n", LMIC.snr/4, rssi);
            // ugh compatibility requires a biased range. RSSI
            LMIC.rssi = (s1_t) (RSSI_OFF + (rssi < -196 ? -196 : rssi > 63 ? 63 : rssi)); // RSSI [dBm] (-196...+63)
        } else if( flags & IRQ_LORA_RXTOUT_MASK ) {
            // indicate timeout
            LMIC.dataLen = 0;
#if LMIC_DEBUG_LEVEL > 0
            ostime_t now2 = os_getTime();
            LMIC_DEBUG_PRINTF("rxtimeout: entry: %"LMIC_PRId_ostime_t" rxtime: %"LMIC_PRId_ostime_t" entry-rxtime: %"LMIC_PRId_ostime_t" now-entry: %"LMIC_PRId_ostime_t" rxtime-txend: %"LMIC_PRId_ostime_t"\n", entry,
                LMIC.rxtime, entry - LMIC.rxtime, now2 - entry, LMIC.rxtime-LMIC.txend);
#endif
        }
        // mask all radio IRQs
        writeReg(LORARegIrqFlagsMask, 0xFF);
        // clear radio IRQ flags
        writeReg(LORARegIrqFlags, 0xFF);
    } else { // FSK modem
        u1_t flags1 = readReg(FSKRegIrqFlags1);
        u1_t flags2 = readReg(FSKRegIrqFlags2);

        LMICOS_logEventUint32("*radio_irq_handler_v2: FSK", ((u2_t)flags2 << 8) | flags1);

        if( flags2 & IRQ_FSK2_PACKETSENT_MASK ) {
            // save exact tx time
            LMIC.txend = now;
        } else if( flags2 & IRQ_FSK2_PAYLOADREADY_MASK ) {
            // save exact rx time
            LMIC.rxtime = now;
            // read the PDU and inform the MAC that we received something
            LMIC.dataLen = readReg(FSKRegPayloadLength);
            // now read the FIFO
            readBuf(RegFifo, LMIC.frame, LMIC.dataLen);
            // read rx quality parameters
            LMIC.snr  = 0;              // SX127x doesn't give SNR for FSK.
            LMIC.rssi = -64 + RSSI_OFF; // SX127x doesn't give packet RSSI for FSK,
                                        // so substitute a dummy value.
        } else if( flags1 & IRQ_FSK1_TIMEOUT_MASK ) {
            // indicate timeout
            LMIC.dataLen = 0;
        } else {
            // ASSERT(0);
            // we're not sure why we're here... treat as timeout.
            LMIC.dataLen = 0;
        }

        // in FSK, we need to put the radio in standby first.
        opmode(OPMODE_STANDBY);
    }
    // go from standby to sleep
    opmode(OPMODE_SLEEP);
    // run os job (use preset func ptr)
    os_setCallback(&LMIC.osjob, LMIC.osjob.func);
#endif /* ! CFG_TxContinuousMode */
}

/*!

\brief Initiate a radio operation.

\param mode Selects the operation to be performed.

The requested radio operation is initiated. Some operations complete
immediately; others require hardware to do work, and don't complete until
an interrupt occurs. In that case, `LMIC.osjob` is scheduled. Because the
interrupt may occur right away, it's important that the caller initialize
`LMIC.osjob` before calling this routine.

- `RADIO_RST` causes the radio to be put to sleep. No interrupt follows;
when control returns, the radio is ready for the next operation.

- `RADIO_TX` and `RADIO_TX_AT` launch the transmission of a frame. An interrupt will
occur, which will cause `LMIC.osjob` to be scheduled with its current
function.

- `RADIO_RX` and `RADIO_RX_ON` launch either single or continuous receives.
An interrupt will occur when a packet is recieved or the receive times out,
which will cause `LMIC.osjob` to be scheduled with its current function.

*/

void os_radio (u1_t mode) {
    switch (mode) {
      case RADIO_RST:
        // put radio to sleep
        opmode(OPMODE_SLEEP);
        break;

      case RADIO_TX:
        // transmit frame now
        LMIC.txend = 0;
        starttx(); // buf=LMIC.frame, len=LMIC.dataLen
        break;

      case RADIO_TX_AT:
        if (LMIC.txend == 0)
            LMIC.txend = 1;
        starttx();
        break;

      case RADIO_RX:
        // receive frame now (exactly at rxtime)
        startrx(RXMODE_SINGLE); // buf=LMIC.frame, time=LMIC.rxtime, timeout=LMIC.rxsyms
        break;

      case RADIO_RXON:
        // start scanning for beacon now
        startrx(RXMODE_SCAN); // buf=LMIC.frame
        break;
    }
}

ostime_t os_getRadioRxRampup (void) {
    return RX_RAMPUP_DEFAULT;
}
