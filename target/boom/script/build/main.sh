#/bin/bash
set -euo pipefail

export TARGET_BUILD_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${TARGET_BUILD_SCRIPT}/variables.sh

${TARGET_BUILD_SCRIPT}/check_envs.sh
${TARGET_BUILD_SCRIPT}/build_target.sh
${TARGET_BUILD_SCRIPT}/build_simulator.sh
