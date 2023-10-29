#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import re
import pandas as pd
from util import add_verilator_public_metacomments

def instrument_coverage_signals(in_file, out_file):
    re_coverage_define = r'(\s*)wire\s+(\[\d+:\d+\])?\s*(\\?coverage[\w_/\[\d\]]+)\s*;'
    re_coverage_assign = r'(\s*)assign\s+(\\?coverage[\w_/\\]+)'

    max_coverage_width = 1
    max_coverage_sum = 0
    coverage_signals = []
    with open(in_file, 'r') as f:
        for line in f.readlines():
            if match := re.match(re_coverage_define, line):
                coverage_signals.append(match[3])
                if match[2]:
                    m = re.match(r'\[(\d+):(\d+)\]', match[2])
                    actual_coverage_width = int(m[1]) - int(m[2]) + 1
                    max_coverage_width = max(max_coverage_width, actual_coverage_width)
                    max_coverage_sum += actual_coverage_width
                logging.info(f'Found coverage signal: {match[3]}')

    coverage_width = 32
    while coverage_width < max_coverage_width:
        coverage_width = coverage_width << 1

    with open(in_file, 'r') as fi, open(out_file, 'w') as fo:
        is_first_define = True
        is_first_assign = True

        for line in fi.readlines():
            if is_first_define and (match := re.match(re_coverage_define, line)):
                coverage_define_line = \
                    f'{match[1]}wire [{coverage_width - 1}:0] coverage [{len(coverage_signals)}] ;\n'
                logging.info(f'Insert define statement: {coverage_define_line}')
                fo.write(coverage_define_line)
                fo.write(line)
                is_first_define = False;
            elif is_first_assign and (match := re.match(re_coverage_assign, line)):
                for i, coverage_signal in enumerate(coverage_signals):
                    coverage_assign_line = f'{match[1]}assign coverage[{i}] = {coverage_signal} ;\n'
                    logging.info(f'Insert assign statement: {coverage_assign_line}')
                    fo.write(coverage_assign_line)
                fo.write(line)
                is_first_assign = False
            else:
                fo.write(line)
    return coverage_signals, coverage_width, ['coverage'], max_coverage_sum

def generate_env_var_file(file, coverage_signals, coverage_width, instrument_log_file, max_coverage_sum):
    code = ''
    code = code + f'FUZZ_BITMAP_INSTANCE_COUNT: {len(coverage_signals)}\n'
    code = code + f'FUZZ_BITMAP_BYTE_SIZE: {int(coverage_width / 8)}\n'
    code = code + f'FUZZ_MAX_COVERAGE: {max_coverage_sum}\n'

    df = pd.read_csv(instrument_log_file, delimiter=',', header=0)
    distance_list = df['distance'].astype(str).tolist()
    distance_string = ",".join(distance_list)
    code = code + f'FUZZ_DIRECTFUZZ_INSTANCE_DIST: {distance_string}\n'

    logging.info(f'Write evn var to file:\n{code}')
    with open(file, 'a') as f:
        f.write(code)

def process(args, common_public_signals):
    coverage_signals, coverage_width, public_signals, max_coverage_sum = \
        instrument_coverage_signals(args.input_verilog, args.output_verilog)
    add_verilator_public_metacomments(args.output_verilog,
                                      public_signals + common_public_signals + ['coverage_target'])
    generate_env_var_file(args.output_fuzz_env, coverage_signals, coverage_width,
                          args.instrument_csv, max_coverage_sum)
