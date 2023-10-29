#!/bin/bash
set -euo pipefail

generate_yosys_script() {
    local INPUT_CODE="$1"
    local NO_INSTRUMENTED_CODE="$2"
    local INSTRUMENTED_CODE="$3"
    local PASS_NAME="$4"
    local PASS_ARGS="$5"

    cat > ${OUT}/instrument.ys << EOF
# read design
read_verilog -sv ${INPUT_CODE};
hierarchy -top Core;

# translate processes to netlists
proc;

# transforms \$pmux cells to trees of \$mux cells to simplify the multiplexer selection signal
pmuxtree;

# FIXME: Special optimization for correct synthesis of rsd
cd MicroOpPicker; opt; opt_clean; cd;
cd \$paramod\$dfb6e54a2c7671936be4f20306af3a7ac7283c02\InterleavedPicker; opt; opt_clean; cd;
opt_muxtree; opt_clean;

# flatten design to analyze across module boundaries
flatten;

# write synthesized design before code instrumentation
write_verilog -noattr ${NO_INSTRUMENTED_CODE};

# run a custom pass for code instrumentation
parse_annotation \
    --output_annotation_type_filepath ${FUZZ_ENV_FILE};
${PASS_NAME} ${PASS_ARGS};

# write synthesized design after code instrumentation
write_verilog -noattr ${INSTRUMENTED_CODE};
EOF
}

source ${TARGET_BUILD_SCRIPT}/variables.sh
generate_yosys_script "$@"
