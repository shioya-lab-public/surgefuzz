#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import shutil
from util import add_verilator_public_metacomments

def process(args, common_public_signals):
    shutil.copyfile(args.input_verilog, args.output_verilog)
    add_verilator_public_metacomments(args.output_verilog, common_public_signals + ['coverage_target'])
