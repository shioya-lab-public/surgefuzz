#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import csv
import logging
import shutil
from util import add_verilator_public_metacomments

def get_inst_list(file):
    signals = []
    signal2width = {}
    with open(file, 'r') as f:
        reader = csv.DictReader(f, delimiter=',')
        for row in reader:
            print(row)
            signal = row["name"]
            width = row["width"]
            signals.append(signal)
            signal2width[signal] = width
    return signals, signal2width

def generate_env_var_file(file, cov_bit, ancestor_count):
    code = ''
    code = code + f'FUZZ_BITMAP_BYTE_SIZE: {1 << int(cov_bit)}\n'
    code = code + f'FUZZ_MAX_COVERAGE: {1 << int(cov_bit)}\n'
    code = code + f'FUZZ_ANCESTOR_COUNT: {ancestor_count}\n'

    logging.info(f'Write evn var to file:\n{code}')
    with open(file, 'a') as f:
        f.write(code)

def process(args, common_public_signals):
    selected_signals, _ = get_inst_list(args.instrument_csv)
    shutil.copyfile(args.input_verilog, args.output_verilog)
    add_verilator_public_metacomments(args.output_verilog, selected_signals + common_public_signals + ['fuzz_ancestors'])
    dependent_count = sum(1 for s in selected_signals if s.startswith("dependent"))
    generate_env_var_file(args.output_fuzz_env, args.coverage_bit_width, dependent_count)
