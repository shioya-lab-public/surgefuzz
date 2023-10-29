#!/bin/bash
set -euo pipefail

build_simulator () {
    pushd ${RSD_ROOT}
    git apply ${PATCH_ROOT}/TestMain.cpp.patch
    popd

    make -C ${RSD_ROOT}/Processor/Src -f Makefile.verilator.mk -j$(nproc) \
        CORE_CODE=$1 \
        ENABLE_SIM_ASSERTION=${ENABLE_SIM_ASSERTION}
}

source ${TARGET_BUILD_SCRIPT}/variables.sh
build_simulator "$@"
