#!/bin/bash
set -euo pipefail

build_simulator () {
    patch -p0 -d ${CHIPYARD}/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/ < ${PATCH_ROOT}/emulator.patch
    make -C ${CHIPYARD}/sims/verilator CONFIG=SmallBoomConfig -j$(nproc)
}

build_simulator "$@"
