#!/bin/bash

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
    echo "    TARGET='foo' $BASH_SOURCE"
elif [ "$TARGET" == "avr" ]
then
    ################################################################################
    # TEST FOR TARGET "avr", i.e. BOARD heltec_wifi_lora_32

    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST'                                                                            platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_eu868 -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST'                                                                            platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_eu868 -D CFG_sx1276_radio' platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-abp/ttn-abp.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LMIC_DEBUG_LEVEL=2 -D LMIC_PRINTF_TO=Serial'                             platformio ci --lib . --board heltec_wifi_lora_32 'examples/ttn-otaa/ttn-otaa.ino'

elif [ "$TARGET" == "esp32" ]
then

    ################################################################################
    # TESTS FOR TARGET "esp32", i.e. BOARD feather32u4

    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST'                                                                              platformio ci --lib . --board feather32u4 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_au921   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/ttn-otaa-feather-us915/ttn-otaa-feather-us915.ino'

    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13'                                                                              platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_eu868   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_au921   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_as923   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_as923jp -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_in866   -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw-feather/raw-feather.ino'

    # Test the raw sketch
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13'                                                                            platformio ci --lib . --board feather32u4 'examples/raw/raw.ino'
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS -D CFG_eu868 -D CFG_sx1276_radio' platformio ci --lib . --board feather32u4 'examples/raw/raw.ino'

    # Make sure debug prints work
    PLATFORMIO_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST -D LED_BUILTIN=13 -D LMIC_DEBUG_LEVEL=2 -D LMIC_PRINTF_TO=Serial' platformio ci --lib . --board feather32u4 'examples/raw/raw.ino'
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
