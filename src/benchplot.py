# Project:  IPIDSurvey-Code
# Filename: benchplot.py
# Authors:  Joshua J. Daymude (jdaymude@asu.edu).

"""
benchplot: Plot the results of the performance benchmark.
"""

from helper import *

import argparse
from cmcrameri import cm
from collections import defaultdict
from itertools import product
import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

mpl.rcParams['lines.linewidth'] = 2


def plot_confband(ax, x, ymean, yerr, color=None, linestyle='-', label=''):
    """
    Plots a solid line (mean) and a shaded confidence band on the given axis.

    :param ax: a matplotlib.axes.Axes object
    :param x: a numeric array of length N representing the x-coordinates
    :param ymean: a numeric array of length N representing the means
    :param yerr: a numeric array of length N representing the stddevs
    :param color: a matplotlib.colors color
    :param linestyle: a string line style for the means line
    :param label: a string legend label for this line
    """
    yabove = [ymean[i] + yerr[i] for i in range(len(ymean))]
    ybelow = [ymean[i] - yerr[i] for i in range(len(ymean))]
    ax.fill_between(x, yabove, ybelow, color=color, alpha=0.2)
    ax.plot(x, ymean, linestyle, color=color, label=label)


def plot_benchmark(results_path, duration=10, max_cpus=128):
    """
    Plot the benchmark results.

    :param results_path: a string path to the benchmark results directory
    :param duration: an int duration per benchmark trial in seconds
    :param max_cpus: an int maximum number of CPUs that were benchmarked
    """
    nums_cpus = np.arange(max_cpus) + 1
    methods = [
        ('global', cm.batlowS(0), '-', 'Globally Inc. (FreeBSD)'),
        ('perconn', cm.batlowS(2), '-', 'Per-Conn. (Linux)'),
        ('perdest32768', cm.batlowS(1), '-',
         r'Per-Dest., $r = 2^{{15}}$ (Windows)'),
        ('perbucketl2048', cm.batlowS(3), '-',
         r'Per-Bucket, $r = 2^{{11}}$ (Linux)'),
        ('perbucketl262144', cm.batlowS(3), '--',
         r'Per-Bucket, $r = 2^{{18}}$ (Linux)'),
        ('perbucketm2048', cm.batlowS(3), ':',
         r'Per-Bucket, $r = 2^{{11}}$ (Linux$^*$)'),
        ('perbucketm262144', cm.batlowS(3), '*',
         r'Per-Bucket, $r = 2^{{18}}$ (Linux$^*$)'),
        ('prngshuffle32768', cm.batlowS(4), '-',
         r'PRNG-IKS, $k = 2^{{15}}$ (OpenBSD)'),
        ('prngqueue8192', cm.batlowS(4), '--',
         r'PRNG-SQ, $k = 2^{{13}}$ (FreeBSD)'),
        ('prngpure', cm.batlowS(4), ':', r'PRNG, $k = 2^{{0}}$ (macOS)')]

    # Load all results data.
    results = defaultdict(dict)
    for (method, _, _, _), num_cpus in product(methods, nums_cpus):
        fname = osp.join(results_path, f'{method}_{num_cpus}.csv')
        results[method][num_cpus] = np.loadtxt(fname, dtype=int, delimiter=',')

    # Compute average IPID request times and IPID throughput rates.
    time_means, time_errs = defaultdict(list), defaultdict(list)
    thru_means, thru_errs = defaultdict(list), defaultdict(list)
    for (method, _, _, _), num_cpus in product(methods, nums_cpus):
        x = results[method][num_cpus][1:]  # Ignore first trial (anomalous).
        time_means[method].append((duration / x).mean())
        if num_cpus > 1:
            time_errs[method].append((duration / x).mean(axis=1).std())
            thru_means[method].append((x.sum(axis=1) / duration).mean())
            thru_errs[method].append((x.sum(axis=1) / duration).std())
        else:
            time_errs[method].append((duration / x).std())
            thru_means[method].append((x / duration).mean())
            thru_errs[method].append((x / duration).std())

    # Plot results.
    fig, ax = plt.subplots(1, 2, figsize=(12, 5), dpi=500, layout='constrained')
    axin = inset_axes(ax[1], width="35%", height="35%", borderpad=1.5)
    for method, color, linestyle, label in methods:
        plot_confband(ax[0], nums_cpus, time_means[method], time_errs[method],
                      color, linestyle, label)
        if method not in ['perconn', 'prngpure']:
            plot_confband(ax[1], nums_cpus, thru_means[method],
                          thru_errs[method], color, linestyle)
        plot_confband(axin, nums_cpus, thru_means[method], thru_errs[method],
                      color, linestyle)

    # Set the axes information and save.
    for axi in ax:
        axi.set(xlim=[1, nums_cpus[-1]])
        axi.set_xscale('log', base=2)
    ax[0].set(title='Ave. IPID Request Time per CPU',
              yscale='log', ylabel='seconds (Log Scale)')
    ax[1].set(title='IPID Throughput', ylabel='IPIDs / second')
    axin.set(xlim=[1, nums_cpus[-1]])
    axin.set_xticks([1, nums_cpus[-1] // 2, nums_cpus[-1]])
    fig.legend(loc='outside right center', fontsize='x-small')
    fig.supxlabel('# Active/Contending CPUs (Log Scale)', x=0.42)
    fig.savefig(osp.join('..', 'figs', 'performance.pdf'))


if __name__ == "__main__":
    # Parse command line arguments.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-R', '--results_path', type=str, default='results',
                        help='Path to benchmark results directory')
    parser.add_argument('-D', '--duration', type=int, default=10,
                        help='Duration of each trial in seconds')
    parser.add_argument('-C', '--max_cpus', type=int, default=128,
                        help='Maximum number of CPUs benchmarked')
    args = parser.parse_args()

    plot_benchmark(args.results_path, args.duration, args.max_cpus)
