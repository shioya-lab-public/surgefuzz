#!/bin/bash
set -euo pipefail

# Function to build a target with coverage instrumentation for fuzzing
build_target() {
    local PASS_ARGS="$1"

    patch -p0 -d ${CHIPYARD}/generators/boom < ${PATCH_ROOT}/fix_store_blocked_counter_bug.patch
    patch -p0 -d ${CHIPYARD}/generators/boom < ${PATCH_ROOT}/revert_stall_load_wakeup_fix.patch
    patch -p0 -d ${CHIPYARD}/generators/boom < ${PATCH_ROOT}/annotation/${ANNOTATION}.patch
    patch -p0 -d ${CHIPYARD}/generators/boom < ${PATCH_ROOT}/debug_pc.patch

    # Generate target verilog
    make -C ${CHIPYARD}/sims/verilator CONFIG=SmallBoomConfig -j$(nproc)

    # Convert $fatal to assert
    sed -e "s@\$fatal@assert(0)@g" \
        ${VERILOG_CODE} > ${VERILOG_CODE_FOR_YOSYS}

    # Generate yosys command
    ${TARGET_BUILD_SCRIPT}/generate_yosys_script.sh ${VERILOG_CODE_FOR_YOSYS} \
        ${NO_INSTRUMENTED_CODE} ${INSTRUMENTED_CODE} \
        "${FUZZER}_cov_inst" "${PASS_ARGS}"

    # Generate a target verilog by yosys
    yosys \
        -s ${OUT}/instrument.ys \
        -m ${PASS_ROOT}/annotation.so \
        -m ${PASS_ROOT}/${FUZZER}.so

    # Add "verilator public" meta comment for fuzzer
    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --instrument_csv ${INST_CSV_FILE} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --input_verilog ${INSTRUMENTED_CODE} \
            --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --coverage_bit_width ${COV_BIT} \
            --common_public_signals "\system.tile_prci_domain.tile_reset_domain.boom_tile.core.fuzz_debug_pc" \
            --debug
    elif [[ "${FUZZER}" == "difuzzrtl" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --input_verilog ${INSTRUMENTED_CODE} \
            --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --coverage_bit_width ${COV_BIT} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --common_public_signals "\system.tile_prci_domain.tile_reset_domain.boom_tile.core.fuzz_debug_pc" \
            --debug
    elif [[ "${FUZZER}" == "directfuzz" ]] || [[ "${FUZZER}" == "rfuzz" ]] || \
         [[ "${FUZZER}" == "blackbox" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --input_verilog ${INSTRUMENTED_CODE} \
            --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --instrument_csv ${OUT}/instrument.csv \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --common_public_signals "\system.tile_prci_domain.tile_reset_domain.boom_tile.core.fuzz_debug_pc" \
            --debug
    fi

    # Convert code for verilator
    verible-verilog-format --column_limit 20000 \
        ${INSTRUMENTED_CODE_WITH_COMMENT} > ${INSTRUMENTED_CODE_FOR_VERILATOR}
    sed -i -e "s@assert(@if (\`STOP_COND) assert(@g" \
        ${INSTRUMENTED_CODE_FOR_VERILATOR}

    # Update files for fuzzing and simulation
    cp ${INSTRUMENTED_CODE_FOR_VERILATOR} ${VERILOG_CODE}
    patch -p0 -d ${CHIPYARD} < ${PATCH_ROOT}/Makefile.patch

    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        ${TARGET_BUILD_SCRIPT}/profile.sh
    fi
}

source ${TARGET_BUILD_SCRIPT}/variables.sh

if [[ "${FUZZER}" == "surgefuzz" ]]; then
    build_target "-w ${INST_CSV_FILE} -max_bit ${SEARCH_BIT} -node_csv ${NODE_CSV} -edge_csv ${EDGE_CSV}"
elif [[ "${FUZZER}" == "difuzzrtl" ]]; then
    build_target "--coverage_bits ${COV_BIT}"
elif [[ "${FUZZER}" == "directfuzz" ]]; then
    build_target "--output_csv ${OUT}/instrument.csv --output_dot ${OUT}/graph.dot --debug"
elif [[ "${FUZZER}" == "rfuzz" ]]; then
    build_target "--output_csv ${OUT}/instrument.csv"
elif [[ "${FUZZER}" == "blackbox" ]]; then
    build_target "--output_csv ${OUT}/instrument.csv"
fi
