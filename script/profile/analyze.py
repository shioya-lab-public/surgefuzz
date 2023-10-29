#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import pandas as pd
from entropy import mi, mic, nmi, entropy
from selector import select_coverage_sigs, select_coverage_sigs_using_mi
import matplotlib.pyplot as plt
import os
from instrument import instrument
import random
import shutil
import graphviz

def calc(df, target_sig_name):
    mic_list = []
    mi_list = []
    nmi_list = []
    entropy_list = []
    dependents = []
    for sig_name in df.columns:
        if target_sig_name in sig_name:
            continue
        x = df[sig_name]
        y = df[target_sig_name]
        dependents.append(sig_name)
        mic_list.append(mic(x, y))
        mi_list.append(mi(x, y))
        nmi_list.append(nmi(x, y))
        entropy_list.append(entropy(x))
    return pd.DataFrame(list(zip(mic_list, mi_list, nmi_list, entropy_list, dependents)), columns =['mic', 'mi', 'nmi', 'entropy', 'name'])

def sort(df, selection_method):
    if selection_method == 'closer':
        df = df.sort_values(['reg_depth', 'depth'], ascending=[True, True]) \
            .reset_index(drop=True)
    elif selection_method == 'closer_mi':
        df = df.sort_values(['reg_depth', 'depth'], ascending=[True, True]) \
            .reset_index(drop=True)
        df_head = df[:1]
        df_tail = df[1:]
        df_tail_filter = df_tail[(0 < df_tail['nmi']) & (df_tail['nmi'] < 0.35)].reset_index(drop=True)
        df = pd.concat([df_head, df_tail_filter]).sort_index()
    else:
        logging.error('Unkown selection_method')
        exit()
    return df

def select_sigs(df, selection_method, cov_bit):
    selected_sigs = []

    # Add an new column
    df['selected_width'] = int(0)

    if selection_method == 'closer' or selection_method == 'closer_mi':
        sum_cov_bit = 0
        for idx, row in df.iterrows():
            if sum_cov_bit + row['width'] < cov_bit:
                # Select all bits of the signal
                selected_sigs.append(row['name'])
                sum_cov_bit = sum_cov_bit + row['width']
                df.at[idx, 'selected_width'] = row['width']
            else:
                # Select a part of the signal
                acceptable_width = cov_bit - sum_cov_bit
                df.at[idx, 'selected_width'] = acceptable_width
                if acceptable_width != row['width']:
                    selected_sigs.append(row['name'] + f'[{acceptable_width-1}:0]')
                else:
                    selected_sigs.append(row['name'])
                break
    else:
        logging.error('Unkown selection_method')
        exit()
    return selected_sigs

def draw_graph(df_nodes, df_edges, out_dir, df):
    g = graphviz.Digraph('depdenent_graph')
    g.attr(rankdir='LR') # lateral direction

    selected_cell_names = df[df['selected_width'] != 0]['cell_name'].values

    for _, row in df_nodes.iterrows():
        if row['type'] in ['dff', 'dlatch']:
            g.node(str(row['id']), shape='doublecircle', color='red')

            # Color the selected register
            if row['name'] in selected_cell_names:
                g.node(str(row['id']), fillcolor='orangered')
                g.node(str(row['id']), style='filled')
        elif row['type'] == 'mux':
            g.node(str(row['id']), shape='diamond')
        else:
            g.node(str(row['id']), shape='ellipse')

    for _, row in df_edges.iterrows():
        g.edge(str(row['id_x']), str(row['id_y']))

    g.render(directory=out_dir)

def read_csv(file):
    df = pd.read_csv(file, delimiter=',', header=0)
    return df

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data_file', required=True, type=str, help='a sampling data of dependent/target registers')
    parser.add_argument('--inst_file', required=True, type=str, help='a list of signal names of dependent registers')
    parser.add_argument('--out_fig_dir', required=False, type=str, help='a directory to save the correlation diagram')
    parser.add_argument('--target_sig_name', required=True, type=str)
    parser.add_argument('--cov_bit', required=True, type=int)
    parser.add_argument('--selection_method', required=True, type=str, \
        choices=['closer', 'closer_mi'])
    parser.add_argument('--seed', required=False, type=int, default=None)
    parser.add_argument('--in_code', required=False, type=str, default=None)
    parser.add_argument('--out_code', required=False, type=str, default=None)
    parser.add_argument('--out_result_file', required=False, type=str, default=None)
    parser.add_argument('--node_csv', required=True, type=str, help='nodes of a circuit graph')
    parser.add_argument('--edge_csv', required=True, type=str, help='edges of a circuit graph')
    parser.add_argument('--out_dir', required=True, type=str, help='output dir')
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()
    return args

def main():
    args = parse_args()
    if args.debug:
        logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(asctime)s %(message)s')
    random.seed(args.seed)

    if args.selection_method != 'blackbox':
        df_inst = read_csv(args.inst_file)
        df_data = read_csv(args.data_file)
        df_infotheo = calc(df_data, args.target_sig_name)
        df = pd.merge(df_infotheo, df_inst, on='name')

        df_sort = sort(df, args.selection_method)
        select_sigs(df_sort, args.selection_method, args.cov_bit)

        if args.selection_method == "closer":
            selected_sigs = select_coverage_sigs(df_inst, df_data, args.target_sig_name, args.cov_bit)
        elif args.selection_method == "closer_mi":
            selected_sigs = select_coverage_sigs_using_mi(df_inst, df_data, args.target_sig_name, args.cov_bit)

        pd.set_option('display.max_rows', None)

        if args.out_result_file:
            df_sort.to_csv(args.out_result_file)

    if args.in_code and args.out_code:
        if args.selection_method != 'blackbox':
            instrument(selected_sigs, args.in_code, args.out_code, args.cov_bit)
        else:
            shutil.copyfile(args.in_code, args.out_code)

    if args.out_fig_dir:
        os.makedirs(args.out_fig_dir, exist_ok=True)
        for signal_name in df_data.columns:
            if 'dependent' not in signal_name:
                continue
            plt.figure()
            plt.scatter(df_data[signal_name], df_data[args.target_sig_name])
            plt.xlabel('dependent')
            plt.ylabel(args.target_sig_name)
            plt.savefig(os.path.join(args.out_fig_dir, signal_name + '.png'))
            plt.close()

    # Draw dependent graph
    df_nodes = read_csv(args.node_csv)
    df_edges = read_csv(args.edge_csv)
    draw_graph(df_nodes, df_edges, args.out_dir, df_sort)

if __name__ == "__main__":
    main()
