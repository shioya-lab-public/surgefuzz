#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import scipy.stats as stats
from sklearn.metrics import mutual_info_score
from minepy import MINE
import math

def entropy(X, base=2):
    _, counts = np.unique(X, return_counts=True)
    return stats.entropy(counts, base=base)

def joint_entropy(X, Y, base=2):
    if not isinstance(X, np.ndarray):
        X = np.array(X)
    if not isinstance(Y, np.ndarray):
        Y = np.array(Y)
    probs = []
    for c1 in set(X):
        for c2 in set(Y):
            probs.append(np.mean(np.logical_and(X == c1, Y == c2)))
    return sum(-p * math.log(p, base) for p in probs if p > 0)

def mutual_information(X, Y):
    return entropy(X) + entropy(Y) - joint_entropy(X, Y)

def mi(X, Y):
    return mutual_info_score(X, Y)

def mic(X, Y):
    mine = MINE(alpha=0.6, c=15, est="mic_approx")
    mine.compute_score(X, Y)
    return mine.mic()

def nmi(X, Y):
    return 2 * mutual_information(X, Y) / (entropy(X) + entropy(Y) + 1e-9)
