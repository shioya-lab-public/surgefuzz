#!/bin/bash
set -euo pipefail

check_envs() {
    if [[ -z "${FUZZ_RND_SEED}" ]] || [[ -z "${FUZZ_MAX_CYCLE}" ]] || \
       [[ -z "${TIMEOUT_SEC}" ]] || [[ -z "${FUZZ_MODE}" ]] || \
       [[ -z "${FUZZ_BUILD_COMMAND}" ]] || [[ -z "${FUZZ_SIM_COMMAND}" ]] || \
       [[ -z "${FUZZ_ASM_HEADER}" ]] || [[ -z "${FUZZ_ASM_FOOTER}" ]];  then
        echo "$FUZZ_RND_SEED, $FUZZ_MAX_CYCLE, $FUZZ_MODE, $FUZZ_BUILD_COMMAND, $FUZZ_SIM_COMMAND,"
        echo "$FUZZ_ASM_HEADER and $FUZZ_ASM_FOOTER must be specified as environment variables."
        exit 1
    fi

    if [ -z $SHARED ] || [ -z $WORKDIR ]; then
        echo '$SHARED must be specified as environment variables.'
        exit 1
    fi
}

check_envs
