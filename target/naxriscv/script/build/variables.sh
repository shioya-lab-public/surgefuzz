VERILOG_CODE=${NAXRISCV}/NaxRiscv.v
NO_INSTRUMENTED_CODE=${OUT}/NaxRiscv_with_no_instrumentation.v
INSTRUMENTED_CODE=${OUT}/NaxRiscv_with_instrumentation.v
INSTRUMENTED_CODE_WITH_COMMENT=${OUT}/NaxRiscv_with_instrumentation_comment.v
COV_INSTRUMENTED_CODE_WITH_COMMENT=${OUT}/NaxRiscv_with_cov_instrumentation_comment.v
COV_INSTRUMENTED_CODE_WITH_COMMENT2=${OUT}/NaxRiscv_with_cov_instrumentation_comment2.v

FUZZ_ENV_FILE=/out/fuzz.env

INST_DAT_FILE=${OUT}/inst_${FUZZER}.dat
NODE_CSV=${OUT}/circuit_graph_node.csv
EDGE_CSV=${OUT}/circuit_graph_edge.csv
