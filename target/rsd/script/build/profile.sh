#!/bin/bash
set -euo pipefail

profile() {
    # Compile verilator codes
    build_simulator_for_profile ${INSTRUMENTED_CODE_WITH_COMMENT}

    local INPUT_FILEPATH="${RSD_ROOT}/Processor/Src/Verification/TestCode/Asm/LoadAndStore"

    # Run a program to measure the relationship between registers
    # The result is written to ${OUT}/profile/dependents.csv
    mkdir -p ${OUT}/profile
    set_envs_from_file ${FUZZ_ENV_FILE}
    make \
        -C /rsd/Processor/Src \
        -f Makefile.verilator.mk run \
        TEST_CODE=${INPUT_FILEPATH}

    local ANALYSIS_LOG=${OUT}/analysis.log
    local ANALYSIS_FIG_DIR=${OUT}/analysis_figs

    python3 ${SCRIPT_ROOT}/profile/analyze.py \
        --data_file ${OUT}/profile/dependents.csv \
        --inst_file ${INST_CSV_FILE} \
        --cov_bit ${COV_BIT} \
        --selection_method ${SELECTION_METHOD} \
        --target_sig_name "coverage_target" \
        --seed 0 \
        --in_code ${INSTRUMENTED_CODE_WITH_COMMENT} \
        --out_code ${COV_INSTRUMENTED_CODE_WITH_COMMENT} \
        --out_result_file ${ANALYSIS_LOG} \
        --out_fig_dir ${ANALYSIS_FIG_DIR} \
        --node_csv ${NODE_CSV} \
        --edge_csv ${EDGE_CSV} \
        --out_dir ${OUT} \
        --debug

    pushd ${RSD_ROOT}
    git apply --reverse ${PATCH_ROOT}/TestMain.cpp.profile.patch
    popd
}

build_simulator_for_profile () {
    pushd ${RSD_ROOT}
    git apply ${PATCH_ROOT}/TestMain.cpp.profile.patch
    popd

    make -C ${RSD_ROOT}/Processor/Src -f Makefile.verilator.mk -j$(nproc) \
        CORE_CODE=$1 \
        ENABLE_SIM_ASSERTION=${ENABLE_SIM_ASSERTION}
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
