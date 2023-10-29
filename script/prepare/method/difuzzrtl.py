#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import re
from util import add_verilator_public_metacomments

def instrument_coverage_signals(in_file, out_file):
    re_coverage_define = r'(\s*)wire\s+(\[\d+:\d+\])?\s*(\\?coverage[\w_/\[\d\]]+)\s*;'
    re_coverage_assign = r'(\s*)assign\s+(\\?coverage[\w_/\\]+)'

    coverage_signals = []
    max_actual_coverage_width = 0
    max_coverage_sum = 0
    with open(in_file, 'r') as f:
        for line in f.readlines():
            if match := re.match(re_coverage_define, line):
                coverage_signals.append(match[3])
                if match[2]:
                    if range_match := re.match(r'\[(\d+):(\d+)\]', match[2]):
                        print(range_match)
                        actual_coverage_width = int(range_match[1]) - int(range_match[2]) + 1
                    else:
                        raise Exception("Unexpected")
                else:
                    actual_coverage_width = 1
                max_actual_coverage_width = max(max_actual_coverage_width, actual_coverage_width)
                max_coverage_sum = max_coverage_sum +  (1 << actual_coverage_width)
                logging.info(f'Found coverage signal: {match[3]}, actual coverage width: {actual_coverage_width}')

    with open(in_file, 'r') as fi, open(out_file, 'w') as fo:
        is_first_define = True
        is_first_assign = True

        for line in fi.readlines():
            if is_first_define and (match := re.match(re_coverage_define, line)):
                coverage_define_line = \
                    f'{match[1]}wire [{max_actual_coverage_width - 1}:0] coverage [{len(coverage_signals)}] ;\n'
                logging.info(f'Insert define statement: {coverage_define_line}')
                fo.write(coverage_define_line)
                fo.write(line)
                is_first_define = False
            elif is_first_assign and (match := re.match(re_coverage_assign, line)):
                for i, coverage_signal in enumerate(coverage_signals):
                    coverage_assign_line = f'{match[1]}assign coverage[{i}] = {coverage_signal} ;\n'
                    logging.info(f'Insert assign statement: {coverage_assign_line}')
                    fo.write(coverage_assign_line)
                fo.write(line)
                is_first_assign = False
            else:
                fo.write(line)

    return coverage_signals, ['coverage'], max_coverage_sum, max_actual_coverage_width

def generate_env_var_file(file, coverage_signals, coverage_bits, max_coverage_sum):
    code = ''
    code = code + f'FUZZ_BITMAP_INSTANCE_COUNT: {len(coverage_signals)};\n'
    code = code + f'FUZZ_BITMAP_BYTE_SIZE: {(1 << int(coverage_bits))};\n'
    code = code + f'FUZZ_MAX_COVERAGE: {max_coverage_sum}\n'

    logging.info(f'Write evn var to file:\n{code}')
    with open(file, 'a') as f:
        f.write(code)

def process(args, common_public_signals):
    coverage_signals, public_signals, max_coverage_sum, actual_coverage_bit_width = \
            instrument_coverage_signals(args.input_verilog, args.output_verilog)
    add_verilator_public_metacomments(args.output_verilog,
                                      public_signals + common_public_signals + ['coverage_target'])
    generate_env_var_file(args.output_fuzz_env, coverage_signals, actual_coverage_bit_width, max_coverage_sum)
