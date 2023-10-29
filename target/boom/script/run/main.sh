#!/bin/bash
set -euo pipefail

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

export TARGET_RUN_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${TARGET_RUN_SCRIPT}/variables.sh

${TARGET_RUN_SCRIPT}/check_envs.sh

set_envs_from_file ${FUZZ_ENV_FILE}

# Execute fuzzer
/fuzzer/fuzz_main \
    --fuzz_dir ${WORKDIR} \
    --fuzz_input_patch ${FUZZ_INPUT_FILEPATH} \
    --fuzz_total_cycle ${FUZZ_MAX_CYCLE} \
    --timeout_sec ${TIMEOUT_SEC} \
    --fuzz_rnd_seed ${FUZZ_RND_SEED} \
    --fuzz_mode ${FUZZ_MODE} \
    --fuzz_gen_command "${FUZZ_GEN_COMMAND}" \
    --fuzz_build_command "${FUZZ_BUILD_COMMAND}" \
    --fuzz_sim_command "${FUZZ_SIM_COMMAND}" \
    --fuzz_asm_header "${FUZZ_ASM_HEADER}" \
    --fuzz_asm_footer "${FUZZ_ASM_FOOTER}" \
    --initial-seed-count 10 \
    --enable-rv64im

# Copy results
cp -f ${WORKDIR}/*.tar.gz ${SHARED}
cp -f ${WORKDIR}/*.csv ${SHARED}
cp -f ${WORKDIR}/*.out ${SHARED}

/bin/bash -i
