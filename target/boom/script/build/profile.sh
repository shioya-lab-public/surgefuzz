#!/bin/bash
set -euo pipefail

profile() {
    # Compile verilator codes
    patch -p0 -d ${CHIPYARD}/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/ < ${PATCH_ROOT}/emulator_for_profile.patch
    make -C ${CHIPYARD}/sims/verilator CONFIG=SmallBoomConfig -j$(nproc)

    local INPUT_FILEPATH="${CHIPYARD}/.conda-env/riscv-tools/riscv64-unknown-elf/share/riscv-tests/isa/rv64ui-p-sw"

    # Run a program to measure the relationship between registers
    # The result is written to ${OUT}/profile/dependents.csv
    mkdir -p ${OUT}/profile
    set_envs_from_file ${FUZZ_ENV_FILE}
    ${CHIPYARD}/sims/verilator/simulator-chipyard-SmallBoomConfig \
        --cycle-count \
        --max-cycles=100000 \
        --seed=0 \
        ${INPUT_FILEPATH}

    local ANALYSIS_LOG=${OUT}/analysis.log
    local ANALYSIS_FIG_DIR=${OUT}/analysis_figs

    pushd ${SCRIPT_ROOT}/profile
    pip install -r requirements.txt;
    popd

    python3 ${SCRIPT_ROOT}/profile/analyze.py \
        --data_file ${OUT}/profile/dependents.csv \
        --inst_file ${INST_CSV_FILE} \
        --cov_bit ${COV_BIT} \
        --selection_method ${SELECTION_METHOD} \
        --target_sig_name "coverage_target" \
        --seed 0 \
        --in_code ${INSTRUMENTED_CODE_FOR_VERILATOR} \
        --out_code "/out/profile_analyze.v" \
        --out_result_file ${ANALYSIS_LOG} \
        --out_fig_dir ${ANALYSIS_FIG_DIR} \
        --node_csv ${NODE_CSV} \
        --edge_csv ${EDGE_CSV} \
        --out_dir ${OUT} \
        --debug
    cp "/out/profile_analyze.v" ${VERILOG_CODE}

    patch -R -p0 -d ${CHIPYARD}/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/ < ${PATCH_ROOT}/emulator_for_profile.patch
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

