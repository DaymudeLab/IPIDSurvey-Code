# Project:  IPIDSurvey-Code
# Filename: helper.py
# Authors:  Joshua J. Daymude (jdaymude@asu.edu).

import numpy as np
import os
import os.path as osp
from scipy.stats import poisson

# Define the maximum number of IP IDs as a global constant.
MAX_IDS = 2**16


def dump_np(fname, arr):
    """
    Writes a numpy array to file.
    """
    os.makedirs(osp.split(fname)[0], exist_ok=True)
    with open(fname, 'wb') as f:
        np.save(f, arr)


def load_np(fname):
    """
    Reads a numpy array from file.
    """
    with open(fname, 'rb') as f:
        return np.load(f)


def positive_pmfs(rate):
    """
    Finds the closed interval of values [n_low, n_high] for the given rate such
    that the Poisson pmfs of all values in the range are positive. Used instead
    of poisson.interval() because of precision issues.

    :param rate: a float Poisson rate of packet transmission
    :returns: an int lower bound of the closed interval of n
    :returns: an int upper bound of the closed interval of n
    :returns: an array of float pmf values for the closed interval
    """
    # Find n_low, the lower bound of the closed interval.
    if poisson.pmf(0, rate) > 0:
        n_low = 0
    else:
        # Perform binary search over the region [0, rate] to find an n_low with
        # pmf(n_low, rate) > 0 and pmf(n_low - 1, rate) ~ 0.
        left, right, mid = 0, int(rate), int(rate) // 2
        while left + 1 < right:
            if poisson.pmf(mid, rate) > 0:
                right = mid
            else:
                left = mid
            mid = (left + right) // 2
        n_low = right

    # Find n_max, a sufficiently large value such that pmf(n_max, rate) ~ 0.
    n_max = max(1, int(rate) * 2)
    while poisson.pmf(n_max, rate) > 0:
        n_max *= 2

    # Find n_high, the upper bound of the closed interval, by performing binary
    # search over the region [rate, n_max] to find an n_high with
    # pmf(n_high, rate) > 0 and pmf(n_high + 1, rate) ~ 0.
    left, right, mid = int(rate), n_max, (int(rate) + n_max) // 2
    while left + 1 < right:
        if poisson.pmf(mid, rate) > 0:
            left = mid
        else:
            right = mid
        mid = (left + right) // 2
    n_high = left

    return n_low, n_high, poisson.pmf(np.arange(n_low, n_high+1), rate)
