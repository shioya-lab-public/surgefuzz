#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import re

def add_verilator_public_metacomments(file, public_signals):
    logging.info(f'Add verilator public metacomments in {public_signals}')
    lines = []
    with open(file, 'r') as f:
        for line in f.readlines():
            match = re.search(r'wire\s+(\[\d+:\d+\])?\s*?(\S+)\s*(\[\d+:\d+\]|\[\d+\])?\s*;$', line)
            if match and match[2] in public_signals:
                new_line = line[:line.index(';')] + ' /* verilator public */ ' + line[line.index(';'):]
                logging.info(f'Add verilator public: {new_line}')
                lines.append(new_line)
            else:
                lines.append(line)

    with open(file, 'w') as f:
        for line in lines:
            f.write(line)
