#!/bin/bash
set -euxo pipefail

openocd \
    -f interface/picoprobe.cfg \
    -f target/rp2040.cfg \
    -c "program quant-proto.elf verify reset exit"

echo "Make sure to reset the pico!"
echo "See https://github.com/raspberrypi/pico-sdk/issues/386"
