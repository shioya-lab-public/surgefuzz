#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import sys
from method import surgefuzz, rfuzz, difuzzrtl, directfuzz, blackbox

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fuzzer', required=True, type=str)
    parser.add_argument('--input_verilog', required=True, type=str)
    parser.add_argument('--output_verilog', required=True, type=str)
    parser.add_argument('--coverage_bit_width', required=False, type=int)
    parser.add_argument('--instrument_csv', required=False, type=str)
    parser.add_argument('--output_fuzz_env', required=True, type=str)
    parser.add_argument('--common_public_signals', nargs='+', default=[])
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()
    return args

def main():
    args = parse_args()

    if args.debug:
        logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(asctime)s %(message)s')

    common_public_signals = args.common_public_signals

    if args.fuzzer == 'surgefuzz':
        surgefuzz.process(args, common_public_signals)
    elif args.fuzzer == 'rfuzz':
        rfuzz.process(args, common_public_signals)
    elif args.fuzzer == 'difuzzrtl':
        difuzzrtl.process(args, common_public_signals)
    elif args.fuzzer == 'directfuzz':
        directfuzz.process(args, common_public_signals)
    elif args.fuzzer == 'blackbox':
        blackbox.process(args, common_public_signals)
    else:
        logging.error("Unknown fuzzer: '{}'. Please specify a valid fuzzer type.".format(args.fuzzer))
        sys.exit(1)

if __name__ == "__main__":
    main()
