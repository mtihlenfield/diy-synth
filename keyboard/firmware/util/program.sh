#!/bin/bash
set -euxo pipefail

openocd \
    -f interface/cmsis-dap.cfg \
    -f target/rp2040.cfg \
    -s tcl \
    -c "program keyboard.elf verify reset exit"

echo "Make sure to reset the pico!"
echo "See https://github.com/raspberrypi/pico-sdk/issues/386"
