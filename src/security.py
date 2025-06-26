# Project:  IPIDSurvey-Code
# Filename: security.py
# Authors:  Joshua J. Daymude (jdaymude@asu.edu).

"""
security: Compute probabilities of an adversary guessing the next IPID for each
          IPID selection method.
"""

from helper import *

import argparse
from cmcrameri import cm
from itertools import repeat
import matplotlib as mpl
import matplotlib.pyplot as plt
from tqdm import tqdm
from tqdm.contrib import tenumerate
from tqdm.contrib.concurrent import process_map

mpl.rcParams['lines.linewidth'] = 2.5


def global_inc(rates, num_guesses):
    """
    Calculates the probability of adversarial guess for globally incrementing
    IPID selection. This probability is the maximum probability over all IPIDs x
    that the next IPID is x. The probability that the next IPID is x, in turn,
    is an infinite sum over n > 0 of terms Pr[n + 1 = x mod 2^16] * pmf(n, rate).
    We deal with the infinite sum by finding the closed interval of n containing
    nearly all of the probability mass; the other terms are negligible.

    :param rates: an array of float Poisson rates of packet transmission
    :param num_guesses: an int number of IDs the adversary gets to guess
    :returns: an array of float probabilities of adversarial guess
    """
    fname = osp.join('results', 'security', 'global_inc.npy')
    try:  # Try to load the pre-computed next ID probabilities from file.
        next_id_probs = load_np(fname)
    except FileNotFoundError:  # If they don't exist, compute and store them.
        next_id_probs = np.zeros((len(rates), MAX_IDS))
        for r, rate in tenumerate(rates):
            # Find the closed interval containing nearly all the probability
            # mass, up to Python's float precision.
            n_low, n_high, pmfs = positive_pmfs(rate)

            # Calculate the probability that the next IPID is any given ID.
            mods = (np.arange(n_low, n_high+1) + 1) % MAX_IDS
            for next_id in range(MAX_IDS):
                for idx in np.where(mods == next_id)[0]:
                    next_id_probs[r][next_id] += pmfs[idx]

        # Write the next ID probabilities to file.
        dump_np(fname, next_id_probs)

    # The adversarial guess probability is the sum of the maximum num_guesses
    # next ID probabilities.
    probs = np.zeros(len(rates))
    top_idxs = np.argpartition(next_id_probs, -num_guesses)[:,-num_guesses:]
    for r, row in enumerate(top_idxs):
        probs[r] = np.sum(next_id_probs[r][row])

    return probs


def per_connection(rates, num_guesses):
    """
    Calculates the probability of adversarial guess for per-connection IPID
    selection.

    :param rates: an array of float Poisson rates of packet transmission
    :param num_guesses: an int number of IDs the adversary gets to guess
    :returns: an array of float probabilities of adversarial guess
    """
    return np.repeat(min(num_guesses / MAX_IDS, 1), len(rates))


def per_destination(rates, num_guesses):
    """
    Calculates the probability of adversarial guess for per-destination IPID
    selection, which is identical to that of globally incrementing selection.

    :param rates: an array of float Poisson rates of packet transmission
    :param num_guesses: an int number of IDs the adversary gets to guess
    :returns: an array of float probabilities of adversarial guess
    """
    return global_inc(rates, num_guesses)


def per_bucket_worker(rate, num_samples, ticks_per_time, seed):
    """
    Estimates the probability of adversarial guess for per-bucket IPID selection
    via simulation for the given rate. This probability is the maximum
    probability over all IPIDs x that the next IPID is x. The probability that
    the next IPID is x, in turn, is an infinite sum over n > 0 of terms
    Pr[sum of n increments = x mod 2^16] * pmf(n, rate). We deal with the
    infinite sum by finding the closed interval of n containing nearly all of
    the probability mass; the other terms are negligible. We then sample many
    IPIDs for each n and use each ID's frequency to estimate its likelihood.

    :param rate: a float Poisson rate of packet transmission
    :param num_samples: an int number of IPID samples per rate/#packets
    :param ticks_per_time: an int number of system ticks per unit time
    :param seed: an int seed for random number generation
    :returns: a (rate, array of next ID probabilities) pair
    """
    # Initialize RNG with same seed for each rate for fair comparison.
    rng = np.random.default_rng(seed)

    # Find the closed interval containing nearly all the probability mass, up to
    # Python's float precision.
    n_low, n_high, pmfs = positive_pmfs(rate)

    # Sample IDs to estimate their likelihood of being the next IPID.
    samples = np.zeros((MAX_IDS, n_high - n_low + 1), dtype=np.int32)
    for _ in range(num_samples):
        deltas = np.maximum(rng.poisson(ticks_per_time / rate, n_high+1), 1)
        incs = rng.integers(1, deltas, endpoint=True)
        next_id = 0
        for n, inc in enumerate(incs):
            next_id = (next_id + inc) % MAX_IDS
            if n >= n_low and n <= n_high:
                samples[next_id][n - n_low] += 1
    next_id_probs = np.sum(samples / num_samples * pmfs, axis=1)

    return (rate, next_id_probs)


def per_bucket(rates, num_guesses, num_samples, ticks_per_time, seed, num_cores):
    """
    Esimates the probability of adversarial guess for per-bucket IPID selection.

    :param rates: an array of float Poisson rates of packet transmission
    :param num_guesses: an int number of IDs the adversary gets to guess
    :param num_samples: an int number of IPID samples per rate/#packets
    :param ticks_per_time: an int number of system ticks per unit time
    :param seed: an int seed for random number generation
    :param num_cores: an int number of processors to parallelize over
    :returns: an array of float probabilities of adversarial guess
    """
    fname = osp.join('results', 'security', 'per_bucket_S' + str(num_samples) +
                     '_R' + str(seed) + '.npy')
    try:  # Try to load the pre-computed next ID probabilities from file.
        next_id_probs = load_np(fname)
    except FileNotFoundError:  # If they don't exist, compute and store them.
        # First, calculate the probability that per-bucket behaves like globally
        # incrementing. In detail, per-bucket samples its increments uniformly
        # at random from {1, ..., Delta}, where Delta is an exponential random
        # variable representing the number of system ticks since the last packet
        # was sent. At high rates, Delta is almost always 1, so the increments
        # are almost always 1, just like globally incrementing.
        n_highs = np.array([positive_pmfs(rate)[1] for rate in rates])
        prob_inc = np.power(1 - np.exp(-1 * rates / ticks_per_time**2), n_highs)

        # Find the fastest rate at which per-bucket behaves differently than
        # globally incrementing with non-negligible probability.
        if prob_inc[-1] < 1:
            # It's possible that it always behaves differently.
            max_rate_idx = len(rates)
        else:
            # Otherwise, we can binary search for it.
            left, right, mid = 0, len(rates) - 1, len(rates) // 2
            while left + 1 < right:
                if prob_inc[mid] < 1:
                    left = mid
                else:
                    right = mid
                mid = (left + right) // 2
            max_rate_idx = right

        # For the rates where per-bucket behaves differently than globally
        # incrementing with non-negigible probability, simulate the per-bucket
        # selection process and report the estimated probabilities.
        p = process_map(per_bucket_worker, rates[:max_rate_idx],
                        repeat(num_samples), repeat(ticks_per_time),
                        repeat(seed), max_workers=num_cores)
        next_id_probs = np.array([x[1] for x in sorted(p, key=lambda x: x[0])])

        # If there are rates at which we can use globally incrementing in place
        # of per-bucket, call global_inc() just to be sure its results exist.
        # Then load its next_id_probs from file and stitch it onto per-bucket's.
        if max_rate_idx < len(rates):
            _ = global_inc(rates, num_guesses)
            global_fname = osp.join('results', 'security', 'global_inc.npy')
            global_next_id_probs = load_np(global_fname)
            next_id_probs = np.append(next_id_probs,
                                      global_next_id_probs[max_rate_idx:],
                                      axis=0)

        # Write the next ID probabilities to file.
        dump_np(fname, next_id_probs)

    # The adversarial guess probability is the sum of the maximum num_guesses
    # next ID probabilities.
    probs = np.zeros(len(rates))
    top_idxs = np.argpartition(next_id_probs, -num_guesses)[:,-num_guesses:]
    for r, row in enumerate(top_idxs):
        probs[r] = np.sum(next_id_probs[r][row])

    return probs


def prng(rates, num_guesses, reserved):
    """
    Calculates the probability of adversarial guess for either PRNG IPID
    selection method (using a searchable queue or an iterated Knuth shuffle)
    with the given number of reserved IPIDs.

    :param rates: an array of float Poisson rates of packet transmission
    :param num_guesses: an int number of IDs the adversary gets to guess
    :param reserved: an int number of IDs stored to reduce collisions
    :returns: an array of float probabilities of adversarial guess
    """
    return np.repeat(min(num_guesses / (MAX_IDS - reserved), 1), len(rates))


def plot_uniform(ax, rates, colors, num_guesses, num_samples, ticks_per_time,
                 seed, num_cores, label=True):
    """
    Plots each IPID selection method's probability of adversarial guess as a
    function of the expected number of packets simultaneously in transit for
    uniform traffic, i.e., when lambda_i = lambda / #IPID resources.

    :param ax: a matplotib.Axes object to plot on
    :param rates: an array of float Poisson rates of packet transmission
    :param colors: a list of five matplotlib colors for the selection methods
    :param num_guesses: an int number of IDs the adversary gets to guess
    :param num_samples: an int number of per-bucket IPID samples per
                        rate/#packets
    :param ticks_per_time: an int number of per-bucket system ticks per unit time
    :param seed: an int seed for random number generation
    :param num_cores: an int number of processors to parallelize over
    :param label: True if and only if plots should be labeled
    """
    tqdm.write("Plotting Adversarial Guess Probabilities " +
               f"(uniform traffic, g={num_guesses})")

    # Plot a vertical line at 2^16 to indicate the maximum # of IPIDs.
    ax.axvline(x=MAX_IDS, linestyle=':', c='k')

    # Globally incrementing has one global counter, so lambda_i = lambda.
    tqdm.write('\tPlotting Globally Incrementing...')
    ax.plot(rates, global_inc(rates, num_guesses), c=colors[0], zorder=2.3)
    if label:
        ax.plot([], [], c=colors[0], label='Globally Inc. (FreeBSD)')

    # Per-connection has as many counters as active connections, but has a
    # constant adversarial guess probability.
    tqdm.write('\tPlotting Per-Connection...')
    ax.plot(rates, per_connection(rates, num_guesses), c=colors[2])
    if label:
        ax.plot([], [], c=colors[2], label='Per-Conn. (Linux)')

    # Per-destination has as many counters as active destinations; Windows sets
    # its purge thresholds at 2^12 (Windows 10) and 2^15 (Windows Server).
    tqdm.write('\tPlotting Per-Destination...')
    prob_perdest = per_destination(rates, num_guesses)
    ax.plot(rates * 2**15, prob_perdest, c=colors[1], zorder=2.2)
    ax.plot(rates * 2**12, prob_perdest, c=colors[1], linestyle='--', zorder=2.19)
    if label:
        ax.plot([], [], c=colors[1],
                label=r'Per-Dest., $r = 2^{{15}}$ (Windows)')
        ax.plot([], [], c=colors[1], linestyle='--',
                label=r'Per-Dest., $r = 2^{{12}}$ (Windows)')

    # Per-bucket has a fixed number of counters based on the machine RAM.
    # We show the lower (2^11) and upper (2^18) bounds.
    tqdm.write('\tPlotting Per-Bucket...')
    prob_perbucket = per_bucket(rates, num_guesses, num_samples,
                                ticks_per_time, seed, num_cores)
    ax.plot(rates * 2**18, prob_perbucket, c=colors[3], zorder=2.1)
    ax.plot(rates * 2**11, prob_perbucket, c=colors[3], linestyle='--', zorder=2.09)
    if label:
        ax.plot([], [], c=colors[3],
                label=r'Per-Bucket, $r = 2^{{18}}$ (Linux)')
        ax.plot([], [], c=colors[3], linestyle='--',
                label=r'Per-Bucket, $r = 2^{{11}}$ (Linux)')

    # PRNG methods have only one resource, so lambda_i = lambda.
    tqdm.write('\tPlotting PRNGs...')
    ax.plot(rates, prng(rates, num_guesses, 32768), c=colors[4])
    ax.plot(rates, prng(rates, num_guesses, 8192), c=colors[4], linestyle='--')
    ax.plot(rates, prng(rates, num_guesses, 0), c=colors[4], linestyle=':')
    if label:
        ax.plot([], [], c=colors[4], label=r'PRNG, $k = 2^{{15}}$ (OpenBSD)')
        ax.plot([], [], c=colors[4], linestyle='--',
                label=r'PRNG, $k = 2^{{13}}$ (FreeBSD)')
        ax.plot([], [], c=colors[4], linestyle=':',
                label=r'PRNG, $k = 2^{{0}}$ (macOS)')


def plot_worst(ax, rates, colors, num_guesses, num_samples, ticks_per_time,
               seed, num_cores):
    """
    Plots each IPID selection method's probability of adversarial guess as a
    function of the expected number of packets simultaneously in transit for
    worst-case traffic, i.e., when lambda_i maximizes adversarial guesses.

    :param ax: a matplotib.Axes object to plot on
    :param rates: an array of float Poisson rates of packet transmission
    :param colors: a list of five matplotlib colors for the selection methods
    :param num_guesses: an int number of IDs the adversary gets to guess
    :param num_samples: an int number of per-bucket IPID samples per
                        rate/#packets
    :param ticks_per_time: an int number of per-bucket system ticks per unit time
    :param seed: an int seed for random number generation
    :param num_cores: an int number of processors to parallelize over
    """
    tqdm.write("Plotting Adversarial Guess Probabilities " +
               f"(worst-case traffic, g={num_guesses})")

    # Plot a vertical line at 2^16 to indicate the maximum # of IPIDs.
    ax.axvline(x=MAX_IDS, linestyle=':', c='k')

    # Globally incrementing has one global counter, so lambda_i = lambda.
    tqdm.write('\tPlotting Globally Incrementing...')
    ax.plot(rates, global_inc(rates, num_guesses), c=colors[0], zorder=2.3)

    # Per-connection has as a constant adversarial guess probability.
    tqdm.write('\tPlotting Per-Connection...')
    ax.plot(rates, per_connection(rates, num_guesses), c=colors[2])

    # Per-destination has as many counters as active destinations, so we
    # find the worst case assuming multiple counters.
    tqdm.write('\tPlotting Per-Destination...')
    prob_perdest = per_destination(rates, num_guesses)
    prob_perdest = [max(prob_perdest[:i+1]) for i in range(len(rates))]
    ax.plot(rates, prob_perdest, c=colors[1], zorder=2.2)

    # Per-bucket has multiple counters, so we find the worst case.
    tqdm.write('\tPlotting Per-Bucket...')
    prob_perbucket = per_bucket(rates, num_guesses, num_samples, ticks_per_time,
                                seed, num_cores)
    prob_perbucket = [max(prob_perbucket[:i+1]) for i in range(len(rates))]
    ax.plot(rates, prob_perbucket, c=colors[3], zorder=2.1)

    # PRNG methods have only one resource, so lambda_i = lambda.
    tqdm.write('\tPlotting PRNGs...')
    ax.plot(rates, prng(rates, num_guesses, 32768), c=colors[4])
    ax.plot(rates, prng(rates, num_guesses, 8192), c=colors[4], linestyle='--')
    ax.plot(rates, prng(rates, num_guesses, 0), c=colors[4], linestyle=':')


def plot_security(num_samples=20*MAX_IDS, ticks_per_time=3, seed=1234567,
                  num_cores=1):
    """
    Plots each IPID selection method's probability of adversarial guess as a
    function of the traffic pattern (uniform vs. worst-case) and the expected
    number of packets simultaneously in transit. Creates both the main figure
    (one adversarial guess) and the appendix figure (multiple guesses).

    :param num_samples: an int number of per-bucket IPID samples per
                        rate/#packets
    :param ticks_per_time: an int number of per-bucket system ticks per unit time
    :param seed: an int seed for random number generation
    :param num_cores: an int number of processors to parallelize over
    """
    # Define total traffic rates and colors for IPID selection methods.
    rates = np.logspace(-28, 26, num=2000, base=2)
    colors = [cm.batlowS(i) for i in range(5)]

    # Plot main figure, set axes information, and save.
    main_fig, main_axes = plt.subplots(1, 2, sharey=True, figsize=(12, 5),
                                       layout='constrained', dpi=500)
    plot_uniform(main_axes[0], rates, colors, 1, num_samples, ticks_per_time,
                 seed, num_cores, label=True)
    plot_worst(main_axes[1], rates, colors, 1, num_samples, ticks_per_time,
               seed, num_cores)
    main_fig.supxlabel(r'$\lambda$, Poisson Rate of Packet Transmission (Log Scale)', x=0.45)
    main_fig.supylabel('Probability of Adversarial Guess (Log Scale)')
    main_fig.legend(loc='outside right center', fontsize='x-small')
    main_axes[0].set(title=r'Uniform Traffic ($\lambda_i = \lambda / r$)',
                     xlim=[2**-10, 2**26], yscale='log',
                     yticks=np.logspace(-5, 0, num=6))
    main_axes[0].set_xscale('log', base=2)
    main_axes[0].set_xticks([2.**i for i in np.arange(-8, 25, 4)])
    main_axes[1].set(title=r'Worst-Case Traffic ($\lambda_i = $argmax Pr[adv. guess])',
                     xlim=[2**-18, 2**18])
    main_axes[1].set_xscale('log', base=2)
    main_axes[1].set_xticks([2.**i for i in np.arange(-16, 17, 4)])
    main_fig.savefig(osp.join('..', 'figs', 'security.pdf'))

    # Plot appendix figure, set axes information, and save.
    apdx_fig, apdx_axes = plt.subplots(2, 3, sharex='row', sharey=True,
                                       figsize=(12, 7), layout='constrained',
                                       dpi=500)
    for i, num_guesses in enumerate([1, 10, 100]):
        plot_uniform(apdx_axes[0, i], rates, colors, num_guesses, num_samples,
                     ticks_per_time, seed, num_cores, label=(i == 0))
        plot_worst(apdx_axes[1, i], rates, colors, num_guesses, num_samples,
                   ticks_per_time, seed, num_cores)
        apdx_axes[0, i].set(title=f"$g = ${num_guesses}")
    apdx_fig.supxlabel(r'$\lambda$, Poisson Rate of Packet Transmission (Log Scale)', x=0.45)
    apdx_fig.supylabel('Probability of Adversarial Guess (Log Scale)')
    apdx_fig.legend(loc='outside right center', fontsize='x-small')
    apdx_axes[0, 0].set(xlim=[2**-10, 2**26],\
                        ylabel=r'Uniform Traffic ($\lambda_i = \lambda / r$)',
                        yscale='log', yticks=np.logspace(-5, 0, num=6))
    apdx_axes[0, 0].set_xscale('log', base=2)
    apdx_axes[0, 0].set_xticks([2.**i for i in np.arange(-8, 25, 8)])
    apdx_axes[1, 0].set(xlim=[2**-18, 2**18],
                        ylabel=r'Worst-Case Traffic ($\lambda_i = $argmax Pr[adv. guess])')
    apdx_axes[1, 0].set_xscale('log', base=2)
    apdx_axes[1, 0].set_xticks([2.**i for i in np.arange(-16, 17, 8)])
    apdx_fig.savefig(osp.join('..', 'figs', 'security_appendix.pdf'))


if __name__ == "__main__":
    # Parse command line arguments.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-S', '--num_samples', type=int, default=20*MAX_IDS,
                        help='Number of per-bucket samples')
    parser.add_argument('-K', '--ticks_per_time', type=int, default=3,
                        help='Number of system ticks per unit time')
    parser.add_argument('-R', '--rand_seed', type=int, default=1234567,
                        help='Seed for random number generation')
    parser.add_argument('-P', '--num_cores', type=int, default=1,
                        help='Number of processors to parallelize over')
    args = parser.parse_args()

    plot_security(args.num_samples, args.ticks_per_time, args.rand_seed,
                  args.num_cores)
