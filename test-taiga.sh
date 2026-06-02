#!/bin/sh

# 1. Start a background timer that will forcefully kill the session after 10 seconds
(sleep 10 && killall river && echo "Fuse blown: Session terminated safely.") &

# 2. Launch your compositor and hand session leadership to your binary
river -c ./target/debug/taiga
