#!/bin/bash

set -ueo pipefail

export PLATFORMIO_SRC_BUILD_FLAGS='-D COMPILE_REGRESSION_TEST'

BOARD_LIST=\
"uno"

EXAMPLE_LIST=\
"raw
raw-halconfig
ttn-abp
ttn-abp-feather-us915-dht22
ttn-otaa
ttn-otaa-feather-us915
ttn-otaa-feather-us915-dht22
ttn-otaa-halconfig-us915
ttn-otaa-network-time"

for board in $BOARD_LIST
do
    for example in $EXAMPLE_LIST
    do
        echo "Testing '$example' with board '$board'"
        platformio ci --lib . --board $board examples/$example
    done
done
