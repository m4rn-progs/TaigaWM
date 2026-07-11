#!/bin/sh
exec gdb -batch \
    -ex "set pagination off" \
    -ex run \
    -ex "bt full" \
    -ex "info locals" \
    --args build/taiga
