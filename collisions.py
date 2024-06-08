# Project:  IPIDSurvey-Code
# Filename: collisions.py
# Authors:  Joshua J. Daymude (jdaymude@asu.edu).

"""
collisions: Compute probabilities of collision across IPID selection methods.
"""

from helper import *

import argparse
from cmcrameri import cm
from itertools import repeat
import matplotlib as mpl
import matplotlib.pyplot as plt
from tqdm import tqdm
from tqdm.contrib.concurrent import process_map

mpl.rcParams['lines.linewidth'] = 2.5


def global_inc(rates):
    """
    Calculates the probability of collision for globally incrementing IPID
    selection, which is conveniently related to the Poisson survival function.

    :param rates: an array of float Poisson rates of packet transmission
    :returns: an array of float probabilities of collision
    """
    return np.array([poisson.sf(MAX_IDS, r) for r in rates])


def per_connection(rates):
    """
    Calculates the probability of collision for per-connection IPID selection,
    which is identical to that of globally incrementing selection.

    :param rates: an array of float Poisson rates of packet transmission
    :returns: an array of float probabilities of collision
    """
    return global_inc(rates)


def per_destination(rates):
    """
    Calculates the probability of collision for per-destination IPID selection,
    which is identical to that of globally incrementing selection.

    :param rates: an array of float Poisson rates of packet transmission
    :returns: an array of float probabilities of collision
    """
    return global_inc(rates)


def per_bucket_worker(rate, num_trials, ticks_per_time, seed):
    """
    Estimates the probability of collision for per-bucket IPIDs selection via
    simulation for the given rate. This probability is an infinite sum over n>0
    of terms Pr[collision | n] * pmf(n, rate). We deal with the infinite sum by
    finding the closed interval of n containing nearly all of the probability
    mass; the other terms are negligible. We estimate the Pr[collision | n]
    terms via averaging the results of simulated trials.

    :param rate: a float Poisson rate of packet transmission
    :param num_trials: an int number of collision trials per rate/#packets
    :param ticks_per_time: an int number of system ticks per unit time
    :param seed: an int seed for random number generation
    :returns: a (rate, probability) pair
    """
    # Initialize RNG with same seed for each rate for fair comparison.
    rng = np.random.default_rng(seed)

    # Find the closed interval containing nearly all the probability mass, up to
    # Python's float precision. Ensure n_low >= 1.
    n_low, n_high, pmfs = positive_pmfs(rate)
    if n_low == 0:
        n_low = 1
        pmfs = pmfs[n_low:]

    # In each collision trial, generate n_high-1 stochastic increments and then
    # test for a collision one increment at a time.
    collisions = np.zeros(n_high - n_low + 1)
    for _ in range(num_trials):
        id, ids = 0, set([0])
        deltas = np.maximum(rng.poisson(ticks_per_time / rate, n_high-1), 1)
        incs = rng.integers(1, deltas, endpoint=True)
        for i, inc in enumerate(incs):
            id = (id + inc) % MAX_IDS
            if id in ids:
                # A collision has occurred after i+1 increments (because i is
                # zero-indexed), which is i+2 packets simultaneously in transit.
                # So count this as a collision for all numbers of packets n that
                # are at least i+2 and are within the interval of interest.
                collisions[max(0, i + 2 - n_low):] += 1
                break
            else:
                ids.add(id)

    return (rate, sum(collisions / num_trials * pmfs))


def per_bucket(rates, num_trials, ticks_per_time, seed, num_cores):
    """
    Estimates the probability of collision for per-bucket IPID selection.

    :param rates: an array of float Poisson rates of packet transmission
    :param num_trials: an int number of collision trials per rate/#packets
    :param ticks_per_time: an int number of system ticks per unit time
    :param seed: an int seed for random number generation
    :param num_cores: an int number of processors to parallelize over
    :returns: an array of float probabilities of collision
    """
    fname = osp.join('results', 'collisions', 'per_bucket_T' + str(num_trials)
                     + '_R' + str(seed) + '.npy')
    try:  # Try to load the pre-computed results from file.
        probs = load_np(fname)
    except FileNotFoundError:  # If they don't exist, compute and store them.
        # Parallelize the workload by rates.
        probs = process_map(per_bucket_worker, rates, repeat(num_trials),
                            repeat(ticks_per_time), repeat(seed),
                            max_workers=num_cores)
        probs = np.array([x[1] for x in sorted(probs, key=lambda x: x[0])])
        dump_np(fname, probs)

    return probs


def prng(rates, reserved):
    """
    Calculates the probability of collision for either PRNG IPID selection
    method (using a searchable queue or an iterated Knuth shuffle) with the
    given number of reserved IPIDs.

    :param rates: an array of float Poisson rates of packet transmission
    :param reserved: an int number of IDs stored to reduce collisions
    :returns: an array of float probabilities of collision
    """
    fname = osp.join('results', 'collisions', 'prng_K' + str(reserved) + '.npy')
    try:  # Try to load the pre-computed results from file.
        probs = load_np(fname)
    except FileNotFoundError:  # If they don't exist, compute and store them.
        # Pre-compute all product terms and partial products for efficiency.
        prod_terms = [1 - i / (MAX_IDS - reserved)
                      for i in np.arange(MAX_IDS - reserved)]
        prod, prods = 1, np.zeros(MAX_IDS - reserved)
        for i in np.arange(MAX_IDS - reserved):
            prod *= prod_terms[i]
            prods[i] = prod

        # Calculate the sum term for each rate using the precomputed products.
        probs = np.zeros(len(rates))
        for r, rate in enumerate(tqdm(rates)):
            pmfs = poisson.pmf(np.arange(MAX_IDS + 1), rate)
            sum_terms = [(1 - prods[n - reserved - 1]) * pmfs[n]
                         for n in np.arange(reserved + 1, MAX_IDS + 1)]
            probs[r] = np.sum(sum_terms) + poisson.sf(MAX_IDS, rate)

        # Write the probabilities to file.
        dump_np(fname, probs)

    return probs


def plot_collisions(num_trials=100000, ticks_per_time=3, seed=1234567,
                    num_cores=1):
    """
    Plots each IPID selection method's probability of collision as a function of
    the expected number of packets simultaneously in transit.

    :param num_trials: an int number of per-bucket collision trials per
                       rate/#packets
    :param ticks_per_time: an int number of per-bucket system ticks per unit time
    :param seed: an int seed for random number generation
    :param num_cores: an int number of processors to parallelize over
    """
    rates = np.logspace(-18, 18, num=1000, base=2)
    colors = [cm.batlowS(i) for i in range(5)]
    fig, ax = plt.subplots(layout='constrained', dpi=500)

    # Plot a vertical line at 2^16 to indicate the maximum # of IPIDs.
    tqdm.write('Plotting MAX_IDS value...')
    ax.axvline(x=MAX_IDS, linestyle=':', c='k')

    # Plot the selection methods' collision probabilities.
    tqdm.write('Plotting Globally Incrementing...')
    ax.plot(rates, global_inc(rates), c=colors[0], linestyle=(0, (7, 21)),
            zorder=2.1)
    ax.plot([], [], c=colors[0], label='Globally Inc. (FreeBSD)')

    tqdm.write('Plotting Per-Connection...')
    ax.plot(rates, per_connection(rates), c=colors[2], linestyle=(7, (7, 21)),
            zorder=2.1)
    ax.plot([], [], c=colors[2], label='Per-Conn. (Linux)')

    tqdm.write('Plotting Per-Destination...')
    ax.plot(rates, per_destination(rates), c=colors[1], linestyle=(14, (7, 21)),
            zorder=2.1)
    ax.plot([], [], c=colors[1], label='Per-Dest. (Windows)')

    tqdm.write('Plotting Per-Bucket...')
    ax.plot(rates, per_bucket(rates, num_trials, ticks_per_time, seed, num_cores),
            c=colors[3], linestyle=(21, (7, 21)), zorder=2.1)
    ax.plot([], [], c=colors[3], label='Per-Bucket (Linux)')

    tqdm.write('Plotting PRNG with k = 4096...')
    ax.plot(rates, prng(rates, 4096), c=colors[4], alpha=0.4,
            label=r'PRNG, $k = 2^{{12}}$ (macOS)')
    tqdm.write('Plotting PRNG with k = 8192...')
    ax.plot(rates, prng(rates, 8192), c=colors[4], alpha=0.7,
            label=r'PRNG, $k = 2^{{13}}$ (FreeBSD)')
    tqdm.write('Plotting PRNG with k = 32768...')
    ax.plot(rates, prng(rates, 32768), c=colors[4], alpha=1,
            label=r'PRNG, $k = 2^{{15}}$ (OpenBSD)')

    # Set titles, scales, and legend.
    ax.set(xlabel=r'$\lambda$, Poisson Rate of Packet Transmission (Log Scale)',
           ylabel='Worst-Case Probability of Collision (Log Scale)',
           yscale='log', yticks=np.logspace(-280, 0, num=8), ylim=[1e-300,None],
           xlim=[2**8, 2**18])
    ax.set_xscale('log', base=2)
    ax.set_xticks([2.**i for i in np.arange(8, 19, 2)])
    ax.set_xticklabels([r'$2^0$', r'$2^{{10}}$', r'$2^{{12}}$', r'$2^{{14}}$',
                        r'$2^{{16}}$', r'$2^{{18}}$'])
    breakargs = dict(marker=[(-1, -.7), (1, .7)], markersize=12, color='w',
                     mec='k', mew=1, clip_on=False)
    ax.plot([0.09, 0.11], [0, 0], transform=ax.transAxes, **breakargs)
    ax.legend(loc='upper left', fontsize='x-small')
    fig.savefig(osp.join('..', 'figs', 'collisions.pdf'))


if __name__ == "__main__":
    # Parse command line arguments.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-T', '--num_trials', type=int, default=100000,
                        help='Number of per-bucket trials')
    parser.add_argument('-K', '--ticks_per_time', type=int, default=3,
                        help='Number of system ticks per unit time')
    parser.add_argument('-R', '--rand_seed', type=int, default=1234567,
                        help='Seed for random number generation')
    parser.add_argument('-P', '--num_cores', type=int, default=1,
                        help='Number of processors to parallelize over')
    args = parser.parse_args()

    plot_collisions(args.num_trials, args.ticks_per_time, args.rand_seed,
                    args.num_cores)
