#!/bin/bash

VERILOG_CODE=${OUT}/Core.v
NO_INSTRUMENTED_CODE=${OUT}/Core_with_no_instrumentation.v
INSTRUMENTED_CODE=${OUT}/Core_with_instrumentation.v
INSTRUMENTED_CODE_WITH_COMMENT=${OUT}/Core_with_instrumentation_comment.v
COV_INSTRUMENTED_CODE_WITH_COMMENT=${OUT}/Core_with_cov_instrumentation_comment.v
ANALYSIS_LOG=${OUT}/analysis.log
ANALYSIS_FIG_DIR=${OUT}/analysis_figs

FUZZ_ENV_FILE=/out/fuzz.env

INST_CSV_FILE=${OUT}/instrument.csv
NODE_CSV=${OUT}/circuit_graph_node.csv
EDGE_CSV=${OUT}/circuit_graph_edge.csv

ENABLE_SIM_ASSERTION=0
