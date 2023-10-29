#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import pandas as pd
from entropy import mi, nmi, entropy

def calc_nmi(df, target_sig_name):
    mi_list = []
    nmi_list = []
    entropy_list = []
    name_list = []
    for sig_name in df.columns:
        if target_sig_name in sig_name:
            continue
        x = df[sig_name]
        y = df[target_sig_name]
        name_list.append(sig_name)
        nmi_list.append(nmi(x, y))
        mi_list.append(mi(x, y))
        entropy_list.append(entropy(x))
    return pd.DataFrame(list(zip(nmi_list, mi_list, entropy_list, name_list)),
                        columns =['nmi', 'mi', 'entropy', 'name'])

def sort_by_distance(df):
    df = df.sort_values(['reg_depth', 'depth'], ascending=[True, True]) \
        .reset_index(drop=True)
    return df

def select_coverage_sigs(df_inst, df_sampling_data, start_target_sig_name, cov_bit):
    df_inst_sorted_by_dist = sort_by_distance(df_inst)
    selected_sig_names = [start_target_sig_name]
    df_nmi = calc_nmi(df_sampling_data, selected_sig_names[-1])
    df_with_nmi = pd.merge(df_inst_sorted_by_dist, df_nmi, on='name')
    print(df_with_nmi)

    sum_cov_bit = df_inst.loc[df_inst['name'] == start_target_sig_name, 'width'].iloc[0]

    for _, row in df_with_nmi.iterrows():
        logging.info(f"Selected a signal: {row['name']}.")
        if sum_cov_bit + row['width'] <= cov_bit:
            # Select all bits of the signal
            selected_sig_names.append(row['name'])
            sum_cov_bit = sum_cov_bit + row['width']
        else:
            # Select a part of the signal
            acceptable_width = cov_bit - sum_cov_bit
            selected_sig_names.append(row['name'] + f'[{acceptable_width-1}:0]')
        if sum_cov_bit == cov_bit:
            break

    logging.info(f"Detected signal names: {selected_sig_names}.")
    return selected_sig_names

def select_coverage_sigs_using_mi(df_inst, df_sampling_data, start_target_sig_name, cov_bit):
    df_inst_sorted_by_dist = sort_by_distance(df_inst)
    selected_sig_names = [start_target_sig_name]
    non_selected_sig_names = []
    sum_cov_bit = df_inst.loc[df_inst['name'] == start_target_sig_name, 'width'].iloc[0]

    while sum_cov_bit < cov_bit:
        logging.info(f"Calculating the mutual information between '{selected_sig_names[-1]}' signal and the remaining candidate signals.")
        df_nmi = calc_nmi(df_sampling_data, selected_sig_names[-1])
        df_candidate = df_inst_sorted_by_dist[~df_inst_sorted_by_dist['name'].isin(selected_sig_names + non_selected_sig_names)]
        print(df_candidate)
        df_with_nmi = pd.merge(df_candidate, df_nmi, on='name')
        print(df_with_nmi)

        logging.info(f"Removing signals with high mutual information.")
        for _, row in df_with_nmi.iterrows():
            if row['nmi'] > 0.7:
                logging.info(f"Not selected a signal: {row['name']}, num: {row['nmi']}.")
                non_selected_sig_names.append(row['name'])

        for _, row in df_with_nmi.iterrows():
            if row['name'] in non_selected_sig_names:
                continue
            logging.info(f"Selected a signal: {row['name']}.")
            if sum_cov_bit + row['width'] <= cov_bit:
                # Select all bits of the signal
                selected_sig_names.append(row['name'])
                sum_cov_bit = sum_cov_bit + row['width']
                break
            else:
                # Select a part of the signal
                acceptable_width = cov_bit - sum_cov_bit
                selected_sig_names.append(row['name'] + f'[{acceptable_width-1}:0]')
                sum_cov_bit = sum_cov_bit + acceptable_width
                break
            assert False, "This code should not be reachable"

    logging.info(f"Detected signal names: {selected_sig_names}.")
    return selected_sig_names

def read_csv(file):
    df = pd.read_csv(file, delimiter=',', header=0)
    return df

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data_file', required=True, type=str, help='a sampling data of dependent/target registers')
    parser.add_argument('--inst_file', required=True, type=str, help='a list of signal names of dependent registers')
    parser.add_argument('--target_sig_name', required=True, type=str)
    parser.add_argument('--cov_bit', required=True, type=int)
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()
    return args

def main():
    args = parse_args()
    if args.debug:
        logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(asctime)s %(message)s')

    df_inst = read_csv(args.inst_file)
    df_sampling_data = read_csv(args.data_file)
    select_coverage_sigs(df_inst, df_sampling_data, args.target_sig_name, args.cov_bit)
    select_coverage_sigs_using_mi(df_inst, df_sampling_data, args.target_sig_name, args.cov_bit)

if __name__ == "__main__":
    main()
