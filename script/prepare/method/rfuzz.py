#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import pandas as pd
import shutil
from util import add_verilator_public_metacomments

def get_coverage_info(instrument_result):
    df = pd.read_csv(instrument_result, delimiter=',', header=0, index_col=0)
    print(df)
    actual_coverage_width = df.at['coverage', 'width']
    print(actual_coverage_width)
    coverage_width = 32
    while coverage_width < actual_coverage_width:
        coverage_width = coverage_width + 32
    instrumented_signals = df.index.values.tolist()
    print(instrumented_signals)
    return instrumented_signals, coverage_width, actual_coverage_width

def generate_env_var_file(file, coverage_width, actual_coverage_width):
    code = ''
    code = code + f'FUZZ_BITMAP_BYTE_SIZE: {int(coverage_width / 8)}\n'
    code = code + f'FUZZ_MAX_COVERAGE: {actual_coverage_width}\n'

    logging.info(f'Write evn var to file:\n{code}')
    with open(file, 'a') as f:
        f.write(code)

def process(args, common_public_signals):
    public_signals, coverage_width, actual_coverage_width= get_coverage_info(args.instrument_csv)
    shutil.copyfile(args.input_verilog, args.output_verilog)
    add_verilator_public_metacomments(args.output_verilog, public_signals + common_public_signals)
    generate_env_var_file(args.output_fuzz_env, coverage_width, actual_coverage_width)
