#!/bin/bash

VERILOG_CODE=${CHIPYARD}/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.top.v
VERILOG_CODE_FOR_YOSYS=${OUT}/SmallBoomConfig.top_for_yosys.v
NO_INSTRUMENTED_CODE=${OUT}/SmallBoomConfig.top.v
INSTRUMENTED_CODE=${OUT}/SmallBoomConfig.top_with_instrumentation.v
INSTRUMENTED_CODE_WITH_COMMENT=${OUT}/SmallBoomConfig.top_with_instrumentation_comment.v
INSTRUMENTED_CODE_FOR_VERILATOR=${OUT}/SmallBoomConfig.top_for_verilator.v

FUZZ_ENV_FILE=/out/fuzz.env

INST_CSV_FILE=${OUT}/instrument.csv
NODE_CSV=${OUT}/circuit_graph_node.csv
EDGE_CSV=${OUT}/circuit_graph_edge.csv
