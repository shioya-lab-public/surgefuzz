#!/bin/bash
set -euo pipefail

check_envs() {
    if [[ -z "${CHIPYARD}" ]] || [[ -z "${SCRIPT_ROOT}" ]] || [[ -z "${PASS_ROOT}" ]] || \
       [[ -z "${FUZZER}" ]] || [[ -z "${SELECTION_METHOD}" ]] || \
       [[ -z "${ANNOTATION}" ]] || [[ -z "${COV_BIT}" ]] || [[ -z "${SEARCH_BIT}" ]] || \
       [[ -z "${MAX_ENERGY}" ]] || [[ -z "${MIN_ENERGY}" ]]; then
        echo "$CHIPYARD, $SCRIPT_ROOT, $PASS_ROOT, $FUZZER, $SELECTION_METHOD, $ANNOTATION, $COV_BIT, $SEARCH_BIT,"
        echo "$MAX_ENERGY and $MIN_ENERGY must be specified as environment variables."
        exit 1
    fi

    if [[ "${FUZZER}" == "surgefuzz" ]]; then
        if [[ ! "${COV_BIT}" =~ ^[0-9]+$ ]] || [[ ! "${SEARCH_BIT}" =~ ^[0-9]+$ ]] || \
        [[ ! "${MAX_ENERGY}" =~ ^[0-9]+$ ]] || [[ ! "${MIN_ENERGY}" =~ ^[0-9]+$ ]]; then
            echo "$COV_BIT, $SEARCH_BIT, $MAX_ENERGY and $MIN_ENERGY must be specified as numeric types."
            exit 1
        fi
        if [[ -z "${POWER_SCHEDULE}" ]]; then
            echo "$POWER_SCHEDULE must be set to ENABLE/DISABLE as environment variables."
            exit 1
        fi
    elif [[ "${FUZZER}" == "difuzzrtl" ]]; then
        if [[ ! "${COV_BIT}" =~ ^[0-9]+$ ]]; then
            echo "$COV_BIT must be specified as numeric types."
            exit 1
        fi
    elif [[ "${FUZZER}" == "directfuzz" ]]; then
        if [[ ! "${MAX_ENERGY}" =~ ^[0-9]+$ ]] || [[ ! "${MIN_ENERGY}" =~ ^[0-9]+$ ]]; then
            echo "$MAX_ENERGY and $MIN_ENERGY must be specified as numeric types."
            exit 1
        fi
    elif [[ "${FUZZER}" == "rfuzz" ]] || [[ "${FUZZER}" == "blackbox" ]]; then
        : # do nothing
    else
        echo "Unsupported fuzzer: ${FUZZER}."
        exit 1
    fi
}

check_envs
