#!/bin/bash

##############################################################################
#
# File: .travis.yml
#
# Function:
#     Travis script for test-building this library.
#
# Copyright Notice:
#     See LICENSE file accompanying this project.
#
# Author:
#     Francesco Zardi   March 2019
#
##############################################################################

# Treat unset variables and parameters as an error
set -o nounset

# Exit immediately if a command fails
set -e

# If set, the return value of a pipeline is the value of the last (rightmost)
# command to exit with a non-zero status, or zero if all commands in the
# pipeline exit successfully
set -o pipefail

if [[ ! -v TARGET ]]
then
    echo "ERROR: variable TARGET is not defined. Please run this scripts as shown next:"
    echo "    TARGET='foo' ${BASH_SOURCE[*]}"
elif [ "$TARGET" == "esp32" ]
then
    ################################################################################
    # TEST FOR TARGET "esp32", i.e. BOARD heltec_wifi_lora_32

    # Compile "ttn-otaa" example in all regions
    PLATFORMIO_BUILD_FLAGS='-D CFG_us915   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_eu868   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_au915   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_as923   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_as923jp -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_kr920   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_in866   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'

    # Compile "ttn-abp" example in all regions
    PLATFORMIO_BUILD_FLAGS='-D CFG_us915   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_eu868   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_au915   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_as923   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_as923jp -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_kr920   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_in866   -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'

    # Compile "ttn-otaa-network-time" example in all regions
    PLATFORMIO_BUILD_FLAGS='-D CFG_us915   -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_eu868   -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_au915   -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_as923   -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_as923jp -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_kr920   -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'
    PLATFORMIO_BUILD_FLAGS='-D CFG_in866   -D CFG_sx1276_radio -D LMIC_ENABLE_DeviceTimeReq=1 -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=Time" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-network-time/ttn-otaa-network-time.ino'


    # Compile "ttn-otaa" example in US with debugging to Serial interface
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LMIC_DEBUG_LEVEL=2 -D LMIC_PRINTF_TO=Serial' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'

    # COMMENTED BECAUSE build fails with
    #   src/raw-feather.ino:119:22: error: 'class HardwareSerial' has no member named 'dtr'
    #
    # # Compile "raw-feather" example in all the regions
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915   -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_eu868   -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_au915   -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_as923   -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_as923jp -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_kr920   -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_in866   -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-feather/raw-feather.ino'

    # Compile "ttn-otaa-feather-us915" example in US and AU regions
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915 -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_au915 -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'

    # Compile "ttn-otaa-feather-us915" example in US region with interrupts
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915 -D CFG_sx1276_radio -D LMIC_USE_INTERRUPTS -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'

    # Compile "ttn-otaa-feather-us915" example in US region with debug to Serial, at level 1 and 2
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915 -D CFG_sx1276_radio -D LMIC_DEBUG_LEVEL=1 -D LMIC_PRINTF_TO=Serial -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915 -D CFG_sx1276_radio -D LMIC_DEBUG_LEVEL=1 -D LMIC_PRINTF_TO=Serial -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'

    # Compile "ttn-otaa-feather-us915-dht22" example in all relevant regions
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915 -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=DHT sensor library, Adafruit Unified Sensor, Wire" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915-dht22/ttn-otaa-feather-us915-dht22.ino'
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_au915 -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=DHT sensor library, Adafruit Unified Sensor, Wire" --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915-dht22/ttn-otaa-feather-us915-dht22.ino'

    # Compile "ttn-abp-feather-us915-dht22" example in all relevant regions with sx1276
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_us915 -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=DHT sensor library, Adafruit Unified Sensor, Wire" --lib . --board heltec_wifi_lora_32 'examples/ttn-abp-feather-us915-dht22/ttn-abp-feather-us915-dht22.ino'
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D CFG_au915 -D CFG_sx1276_radio -D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS' platformio ci --project-option="lib_deps=DHT sensor library, Adafruit Unified Sensor, Wire" --lib . --board heltec_wifi_lora_32 'examples/ttn-abp-feather-us915-dht22/ttn-abp-feather-us915-dht22.ino'

    # COMMENTED BECAUSE build fails with
    #   src/raw-feather.ino:119:22: error: 'class HardwareSerial' has no member named 'dtr'
    #
    # # Compile "raw-halconfig" example in US
    # PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D COMPILE_REGRESSION_TEST' platformio ci --lib . --board heltec_wifi_lora_32 'examples/raw-halconfig/raw-halconfig.ino'

    # Compile "ttn-otaa-halconfig-us915" example in US
    PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4 -D COMPILE_REGRESSION_TEST' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-halconfig-us915/ttn-otaa-halconfig-us915.ino'

    # Expect failure when compiling some examples without the COMPILE_REGRESSION_TEST flag
    if [ "$(PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'                                                                                     1>&2; echo $?)" -eq 0 ]; then echo "ERROR: Compilation did not fail!"; exit 1; fi
    if [ "$(                                                    platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'                                                                                                                 1>&2; echo $?)" -eq 0 ]; then echo "ERROR: Compilation did not fail!"; exit 1; fi
    if [ "$(                                                    platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'                                                                                                                   1>&2; echo $?)" -eq 0 ]; then echo "ERROR: Compilation did not fail!"; exit 1; fi
    if [ "$(PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4' platformio ci --lib . --board heltec_wifi_lora_32 --project-option="lib_deps=DHT sensor library, Adafruit Unified Sensor, Wire" 'examples/ttn-otaa-feather-us915-dht22/ttn-otaa-feather-us915-dht22.ino' 1>&2; echo $?)" -eq 0 ]; then echo "ERROR: Compilation did not fail!"; exit 1; fi
    if [ "$(PLATFORMIO_BUILD_FLAGS='-D ARDUINO_AVR_FEATHER32U4' platformio ci --lib . --board heltec_wifi_lora_32 --project-option="lib_deps=DHT sensor library, Adafruit Unified Sensor, Wire" 'examples/ttn-abp-feather-us915-dht22/ttn-abp-feather-us915-dht22.ino'   1>&2; echo $?)" -eq 0 ]; then echo "ERROR: Compilation did not fail!"; exit 1; fi

    # Expect failure when compiling for more than one radio
    if [ "$(PLATFORMIO_BUILD_FLAGS='-D CFG_sx1272_radio -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino' 1>&2; echo $?)" -eq 0 ]; then echo "ERROR: Compilation did not fail!"; exit 1; fi

elif [ "$TARGET" == "avr" ]
then

    ################################################################################
    # TESTS FOR TARGET "avr", i.e. BOARD feather32u4

    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST'                                                                              platformio ci --lib . --board feather32u4 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_au915   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'

    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13'                                                                              platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_eu868   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_au915   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_as923   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_as923jp -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_kr920   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_in866   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'

    # Test the raw sketch
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13'                                                                            platformio ci --lib . --board feather32u4 'examples/raw/raw.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_eu868 -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw/raw.ino'

    # Make sure debug prints work
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D LMIC_DEBUG_LEVEL=2 -D LMIC_PRINTF_TO=Serial' platformio ci --lib . --board feather32u4 'examples/raw/raw.ino'

    # Check build with deprecated CFG_au921 flag
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_au921   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'
elif [ "$TARGET" == "samd" ]
then
    echo "WARNING: target '$TARGET' is not configured yet."
elif [ "$TARGET" == "stm32l0" ]
then
    echo "WARNING: target '$TARGET' is not configured yet."
else
    echo "ERROR: target '$TARGET' is not supported"
    exit 1
fi
