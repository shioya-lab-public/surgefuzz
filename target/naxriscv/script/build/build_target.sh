#!/bin/bash
set -euo pipefail

# Function to build a target with coverage instrumentation for fuzzing
build_target() {
    local PASS_ARGS="$1"

    # Generate Naxriscv.v by SpinalHDL
    generate_naxriscv

    ${TARGET_BUILD_SCRIPT}/generate_yosys_script.sh ${VERILOG_CODE} \
        ${NO_INSTRUMENTED_CODE} ${INSTRUMENTED_CODE} \
        "${FUZZER}_cov_inst" "${PASS_ARGS}"

    # Generate a target verilog by yosys
    yosys \
        -s ${OUT}/instrument.ys \
        -m ${PASS_ROOT}/annotation.so \
        -m ${PASS_ROOT}/${FUZZER}.so

    # Add "verilator public" meta comment for original simulator
    python3 /script/verilator_public.py \
        --input_original_verilog /NaxRiscv/NaxRiscv.v \
        --input_verilog ${INSTRUMENTED_CODE} \
        --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT}

    # Add "verilator public" meta comment for fuzzer
    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --instrument_csv ${INST_DAT_FILE} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --input_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --output_verilog ${COV_INSTRUMENTED_CODE_WITH_COMMENT} \
            --coverage_bit_width ${COV_BIT} \
            --debug
    elif [[ "${FUZZER}" == "difuzzrtl" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --input_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --output_verilog ${COV_INSTRUMENTED_CODE_WITH_COMMENT} \
            --coverage_bit_width ${COV_BIT} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --debug
    elif [[ "${FUZZER}" == "directfuzz" ]] || [[ "${FUZZER}" == "rfuzz" ]] || \
         [[ "${FUZZER}" == "blackbox" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --input_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --output_verilog ${COV_INSTRUMENTED_CODE_WITH_COMMENT} \
            --instrument_csv ${OUT}/instrument.csv \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --debug
    fi

    add_needed_module ${COV_INSTRUMENTED_CODE_WITH_COMMENT}
    cp ${COV_INSTRUMENTED_CODE_WITH_COMMENT} ${OUT}/NaxRiscv.v

    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        ${TARGET_BUILD_SCRIPT}/profile.sh
        cp ${COV_INSTRUMENTED_CODE_WITH_COMMENT2} ${OUT}/NaxRiscv.v
    fi
}

generate_naxriscv() {
    patch -p0 -d ${NAXRISCV} < ${PATCH_ROOT}/makefile.patch
    patch -p0 -d ${NAXRISCV} < ${PATCH_ROOT}/annotation/${ANNOTATION}.patch
    pushd ${NAXRISCV}
    # Generate NaxRiscv.v
    sbt "runMain naxriscv.Gen"
    popd
}

add_needed_module() {
    local FILE="$1"

    # Add a module to build target correctly
    cat >> ${FILE} << EOF
module RamAsyncMwXor_1 (
  input               io_writes_0_valid,
  input      [3:0]    io_writes_0_payload_address,
  input               io_writes_1_valid,
  input      [3:0]    io_writes_1_payload_address,
  input               io_read_0_cmd_valid,
  input      [3:0]    io_read_0_cmd_payload,
  input               clk,
  input               reset
);
endmodule
EOF
}

source ${TARGET_BUILD_SCRIPT}/variables.sh

if [[ "${FUZZER}" == "surgefuzz" ]]; then
    build_target "-w ${INST_DAT_FILE} -max_bit ${SEARCH_BIT} -node_csv ${NODE_CSV} -edge_csv ${EDGE_CSV}"
elif [[ "${FUZZER}" == "difuzzrtl" ]]; then
    build_target "--coverage_bits ${COV_BIT}"
elif [[ "${FUZZER}" == "directfuzz" ]]; then
    build_target "--output_csv ${OUT}/instrument.csv --output_dot ${OUT}/graph.dot --debug"
elif [[ "${FUZZER}" == "rfuzz" ]]; then
    build_target "--output_csv ${OUT}/instrument.csv"
elif [[ "${FUZZER}" == "blackbox" ]]; then
    build_target "--output_csv ${OUT}/instrument.csv"
fi
