# Project:  IPIDSurvey-Code
# Filename: phases.py
# Authors:  Joshua J. Daymude (jdaymude@asu.edu).

"""
phases: Plot recommended IPID selection methods as a function of traffic rates.
"""

from helper import *

from cmcrameri import cm
from matplotlib.patches import Polygon
import matplotlib.pyplot as plt


if __name__ == "__main__":
    fig, ax = plt.subplots(figsize=(5, 4.75), dpi=500, tight_layout=True)

    ax.add_patch(Polygon([(2**-10, 2**-10), (2**0, 2**0), (2**0, 2**-10)],
                         ec='w', fc=cm.batlowS(4), label='PRNG (all)'))
    ax.add_patch(Polygon([(2**0, 2**-10), (2**0, 2**0),
                          (2**10, 2**0), (2**10, 2**-10)],
                         hatch='\\\\\\', ec='w', fc=cm.batlowS(4),
                         label='PRNG (non-CB) + Per-Conn. (CB)'))
    ax.add_patch(Polygon([(2**0, 2**0), (2**10, 2**10), (2**10, 2**0)],
                         hatch='\\\\\\', ec='w', fc=cm.batlowS(3),
                         label='Per-Bucket (non-CB) + Per-Conn. (CB)'))
    ax.add_patch(Polygon([(2**10, 2**-10), (2**10, 2**10),
                          (2**20, 2**10), (2**20, 2**-10)],
                         ec='w', fc=cm.batlowS(0), label='Globally Inc. (all)'))
    ax.add_patch(Polygon([(2**10, 2**10), (2**20, 2**20), (2**20, 2**10)],
                         hatch='\\\\\\', ec='w', fc=cm.batlowS(0),
                         label='Globally Inc. (non-CB) + Per-Conn. (CB)'))

    ax.set(xlim=[2**-10, 2**20], ylim=[2**-10, 2**20],
           xlabel=r'$\lambda$, Total Outgoing Packet Rate (Log Scale)',
           ylabel=r'$\lambda_n$, Non-Connection-Bound Packet Rate (Log Scale)')
    ax.set_xscale('log', base=2)
    ax.set_xticks([2.**i for i in np.arange(-10, 21, 10)])
    ax.set_xticklabels(['0', r'$2^0$', r'$2^{10}$', r'$\infty$'])
    ax.set_yscale('log', base=2)
    ax.set_yticks([2.**i for i in np.arange(-10, 21, 10)])
    ax.set_yticklabels(['0', r'$2^0$', r'$2^{10}$', r'$\infty$'])
    ax.yaxis.set_label_position('right')
    ax.yaxis.tick_right()
    ax.legend(loc='upper left', fontsize='small')
    ax.spines[['left', 'top']].set_visible(False)

    fig.savefig(osp.join('..', 'figs', 'phases.pdf'))
