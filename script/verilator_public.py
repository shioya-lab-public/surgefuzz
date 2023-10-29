#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import re

def get_public_signals(file):
    re_define = r'(wire|reg|input|output)\s+(\[\d+:\d+\])?\s*?(\S+)\s*(\[\d+:\d+\]|\[\d+\])?'
    public_signals = []
    with open(file, 'r') as f:
        for line in f.readlines():
            if 'verilator public' not in line:
                continue
            if match := re.search(re_define, line):
                logging.debug(f'Found verilator public in {file}: {match[3]}.')
                public_signals.append(match[3])
    return public_signals

def add_verilator_public(in_file, out_file, public_signals):
    logging.info(f'Add verilator public metacomments in {public_signals}.')
    lines = []
    with open(in_file, 'r') as f:
        for line in f.readlines():
            match = re.search(r'(wire|reg)\s+(\[\d+:\d+\])?\s*?(\S+)\s*(\[\d+:\d+\]|\[\d+\])?\s*;$', line)
            if match and match[3] in public_signals:
                new_line = line[:line.index(';')] + ' /* verilator public */ ' + line[line.index(';'):]
                logging.info(f'Add verolator public: {new_line}.')
                lines.append(new_line)
            else:
                lines.append(line)

    with open(out_file, 'w') as f:
        for line in lines:
            f.write(line)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_original_verilog', required=True, type=str)
    parser.add_argument('--input_verilog', required=True, type=str)
    parser.add_argument('--output_verilog', required=True, type=str)
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()
    return args

def main():
    args = parse_args()

    if args.debug:
        logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(asctime)s %(message)s')

    public_signals = get_public_signals(args.input_original_verilog)
    print(public_signals)
    add_verilator_public(args.input_verilog, args.output_verilog, public_signals)

if __name__ == "__main__":
    main()
