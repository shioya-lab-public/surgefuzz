#!/bin/bash
set -euo pipefail

build_simulator() {
    pushd ${NAXRISCV}/src/test/cpp/naxriscv

    patch -p0 -d ${NAXRISCV} < ${PATCH_ROOT}/main.patch

    # Compile verilator codes
    make compile \
        NAXRISCV_VERILOG=${OUT}/NaxRiscv.v \
        FUZZER=${FUZZER} \
        ALLOCATOR_CHECKS=no

    popd
}

source ${TARGET_BUILD_SCRIPT}/variables.sh
build_simulator "$@"
