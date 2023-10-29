#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import logging

def instrument(selected_sigs, in_file, out_file, cov_bit):
    with open(in_file, 'r') as fi, open(out_file, 'w') as fo:
        for line in fi.readlines():
            if match := re.match(r'(\s*)wire\s+(\[\d+:\d+\]\s+)?coverage\s.*;$', line):
                new_line = f'{match[1]}wire [{cov_bit - 1}:0] coverage /*verilator public*/ ;\n'
                logging.info(f'Rewrite from\n{line}to\n{new_line}')
                fo.write(new_line)
            elif match := re.match(r'(\s*)assign\s+coverage\s.*;$', line):
                selected_sigs_concat = ','.join(selected_sigs)
                new_line = f'{match[1]}assign coverage = {{{selected_sigs_concat}}};\n'
                logging.info(f'Rewrite from\n{line}to\n{new_line}')
                fo.write(new_line)
            else:
                fo.write(line)
