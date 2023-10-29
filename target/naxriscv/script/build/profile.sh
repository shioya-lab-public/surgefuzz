#!/bin/bash
set -euo pipefail

profile() {
    patch -p0 -d ${NAXRISCV} < ${PATCH_ROOT}/main_for_profile.patch

    pushd ${NAXRISCV}/src/test/cpp/naxriscv
    # Compile verilator codes
    make compile \
        NAXRISCV_VERILOG=${OUT}/NaxRiscv.v \
        FUZZER=${FUZZER} \
        ALLOCATOR_CHECKS=no
    popd

    local NAXSOFTWARE_ROOT=/tmp/NaxSoftware
    git clone https://github.com/SpinalHDL/NaxSoftware ${NAXSOFTWARE_ROOT}

    if [[ "$ANNOTATION" == "branch_miss" ]]; then
        local INPUT_FILEPATH="${RISCV_TESTS}/isa/rv32ui-p-bge"
    else
        local INPUT_FILEPATH="${RISCV_TESTS}/isa/rv32ui-p-sw"
    fi

    # Run a program to measure the relationship between registers
    # The result is written to ${OUT}/profile/dependents.csv
    mkdir -p ${OUT}/profile
    set_envs_from_file ${FUZZ_ENV_FILE}
    ${NAXRISCV}/src/test/cpp/naxriscv/obj_dir/VNaxRiscv \
        --load-elf ${INPUT_FILEPATH} \
        --pass-symbol=pass \
        --seed=0 \
        --timeout=100000 \
        --stats-print-all \
        --spike-disable \
        --output-dir=${WORKDIR}

    local ANALYSIS_LOG=${OUT}/analysis.log
    local ANALYSIS_FIG_DIR=${OUT}/analysis_figs

    python3 ${SCRIPT_ROOT}/profile/analyze.py \
        --data_file ${OUT}/profile/dependents.csv \
        --inst_file ${INST_DAT_FILE} \
        --cov_bit ${COV_BIT} \
        --selection_method ${SELECTION_METHOD} \
        --target_sig_name "coverage_target" \
        --seed 0 \
        --in_code ${COV_INSTRUMENTED_CODE_WITH_COMMENT} \
        --out_code ${COV_INSTRUMENTED_CODE_WITH_COMMENT2} \
        --out_result_file ${ANALYSIS_LOG} \
        --out_fig_dir ${ANALYSIS_FIG_DIR} \
        --node_csv ${NODE_CSV} \
        --edge_csv ${EDGE_CSV} \
        --out_dir ${OUT} \
        --debug

    patch -R -p0 -d ${NAXRISCV} < ${PATCH_ROOT}/main_for_profile.patch
}

# Function to set environment variables from a file
set_envs_from_file() {
    local FILE="$1"

    # Read the file line by line
    while IFS= read -r line; do
        # Split by ":" to get the key and the value
        local key=$(echo $line | cut -d: -f1 | tr -d ' ')
        local value=$(echo $line | cut -d: -f2- | sed 's/^ *//')  # Remove leading spaces from the value

        # Ensure that the key is valid before setting
        if [[ -n $key ]]; then
            # Set the environment variable
            export "$key=$value"
        else
            echo "Warning: Invalid line detected: $line"
        fi
    done < "$FILE"
}

source ${TARGET_BUILD_SCRIPT}/variables.sh
profile
