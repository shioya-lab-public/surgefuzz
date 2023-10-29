#!/bin/bash
set -euo pipefail

# Function to build a target with coverage instrumentation for fuzzing
build_target() {
    local PASS_ARGS="$1"

    prepare

    ${TARGET_BUILD_SCRIPT}/generate_yosys_script.sh ${VERILOG_CODE} \
        ${NO_INSTRUMENTED_CODE} ${INSTRUMENTED_CODE} \
        "${FUZZER}_cov_inst" "${PASS_ARGS}"

    # Generate a target verilog by yosys
    yosys \
        -s ${OUT}/instrument.ys \
        -m ${PASS_ROOT}/annotation.so \
        -m ${PASS_ROOT}/${FUZZER}.so

    # Generate files for verilator sim
    patch -o ${OUT}/RAM_with_comment.v ${OUT}/RAM.v /patch/RAM.v.patch

    # Add "verilator public" meta comment for fuzzer
    local COMMON_PUBLIC_SIGNALS="\\activeList.headPtr \\cmStage.commit"
    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --instrument_csv ${INST_CSV_FILE} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --input_verilog ${INSTRUMENTED_CODE} \
            --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --coverage_bit_width ${COV_BIT} \
            --common_public_signals ${COMMON_PUBLIC_SIGNALS} \
            --debug
    elif [[ "${FUZZER}" == "difuzzrtl" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --input_verilog ${INSTRUMENTED_CODE} \
            --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --coverage_bit_width ${COV_BIT} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --common_public_signals ${COMMON_PUBLIC_SIGNALS} \
            --debug
    elif [[ "${FUZZER}" == "directfuzz" ]] || [[ "${FUZZER}" == "rfuzz" ]] || \
         [[ "${FUZZER}" == "blackbox" ]]; then
        python3 ${SCRIPT_ROOT}/prepare/main.py \
            --fuzzer ${FUZZER} \
            --input_verilog ${INSTRUMENTED_CODE} \
            --output_verilog ${INSTRUMENTED_CODE_WITH_COMMENT} \
            --instrument_csv ${INST_CSV_FILE} \
            --output_fuzz_env ${FUZZ_ENV_FILE} \
            --common_public_signals ${COMMON_PUBLIC_SIGNALS} \
            --debug
    fi

    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        ${TARGET_BUILD_SCRIPT}/profile.sh
    else
        cp ${INSTRUMENTED_CODE_WITH_COMMENT} ${COV_INSTRUMENTED_CODE_WITH_COMMENT}
    fi
}

prepare () {
    pushd ${RSD_ROOT}
    git apply ${PATCH_ROOT}/ReadyBitTable.sv.patch
    git apply ${PATCH_ROOT}/ResetController.sv.patch
    git apply ${PATCH_ROOT}/Makefile.verilator.mk.patch
    git apply ${PATCH_ROOT}/MemoryMapTypes.sv.patch
    popd

    for patch_file in ${PATCH_ROOT}/assertion/*; do
        echo "Applying ${patch_file}"
        patch -p1 -d ${RSD_ROOT} < $patch_file
    done

    if [ -e ${PATCH_ROOT}/annotation/${ANNOTATION}.patch ]; then
        patch -p1 -d ${RSD_ROOT} < ${PATCH_ROOT}/annotation/${ANNOTATION}.patch
    else
        echo "Invalid $ANNOTATION"
        exit 1
    fi

    make -C /rsd/Processor/Src/Verification/TestCode/ clean
    make -C /rsd/Processor/Src/Verification/TestCode/
    make -C /rsd/Processor/Src/Verification/TestCode/Asm/

    python3 ${SCRIPT_ROOT}/sv2v.py \
        --target_root_dir ${RSD_ROOT} \
        --workdir ${OUT} \
        --debug
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
