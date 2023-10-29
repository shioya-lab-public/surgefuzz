#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import os
import re
import subprocess
import sys

def search_filepath(target_root_dir):
    rsd_path = os.path.abspath(
        os.path.join(target_root_dir, "Processor", "Src"))
    rsd_core_make = os.path.join(rsd_path, "Makefiles", "CoreSources.inc.mk")

    sources = []
    incdir = os.path.join(rsd_path, "")

    with open(rsd_core_make, "r") as f:
        in_source_list = False
        for line in f.readlines():
            line = re.sub(r"#.+[\n\r]$", "", line)  # Remove comments

            # Find lines starting with "TYPES|CORE_MODULES=" and not ending with "\".
            m = re.search(r"^(TYPES|CORE_MODULES)[\s]*=(.+)$", line)
            if m:
                line = m.group(2)  # Remove TYPES|CORE_MODULES=
                in_source_list = True

            if in_source_list:
                # Find the end of source list
                m = re.search(r"\\[\n\r]*$", line)
                if not m:
                    in_source_list = False
                # Remove backslash and spaces
                line = re.sub(r"\\[\n\r]*$", "", line)
                line = line.strip()
                if (line != ""):
                    for i in line.split(r"\s+"):
                        if i == "Primitives/RAM.sv" or i == "Memory/Memory.sv":
                            continue
                        sources.append(os.path.join(rsd_path, i))

    logging.info(f"Searched sv2v sources is {sources}.")
    logging.info(f"Searched sv2v incdir is {incdir}.")
    return sources, incdir

def execute_sv2v_command(sources, incdir, target_root_dir, workdir):
    try:
        cmd = [
            'sv2v',
            '-I', incdir,
            '-w', os.path.join(workdir, 'Core.v'),
            '-E', 'Assert',
            '-D', 'RSD_FUNCTIONAL_SIMULATION',
            '-D', 'RSD_MARCH_INT_ISSUE_WIDTH=2',
        ]
        cmd = cmd + sources
        logging.info(f"Run command: {cmd}.")
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        logging.error(f'Failed to execute {cmd}.')
        sys.exit(1)

    try:
        cmd = [
            'sv2v', os.path.join(target_root_dir, 'Processor', 'Src', 'Primitives', 'RAM.sv'),
            '-I', incdir,
            '-w', os.path.join(workdir, 'RAM.v'),
            '-E', 'Assert',
            '-D', 'RSD_FUNCTIONAL_SIMULATION',
            '-D', 'RSD_MARCH_INT_ISSUE_WIDTH=2'
        ]
        logging.info(f"Run command: {cmd}.")
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        logging.error(f'Failed to execute {cmd}.')
        sys.exit(1)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target_root_dir', required=True, type=str)
    parser.add_argument('--workdir', required=True, type=str)
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()
    return args

def main():
    args = parse_args()

    if args.debug:
        logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(asctime)s %(message)s')

    logging.info('Started converting system verilog to verilog.')

    sources, incdir = search_filepath(args.target_root_dir)
    execute_sv2v_command(sources, incdir, args.target_root_dir, args.workdir)

    logging.info('Completed converting system verilog to verilog.')

if __name__ == "__main__":
    main()
