#!/bin/bash

#SBATCH -N 1                          # Number of nodes.
#SBATCH -n 28                         # Number of cores per node.
#SBATCH -t 7-00:00                    # Wall clock time for job (D-HH:MM)
#SBATCH -o logs/%j.out                # STDOUT (%j = JOBID)
#SBATCH -e logs/%j.err                # STDERR (%j = JOBID)
#SBATCH --mail-type=FAIL,END          # Send start/stop/failure notifications.
#SBATCH --mail-user=jdaymude@asu.edu  # Address to send notifications to.

# Run this script with:
# sbatch agave_security.sh

# Load python environment.
module purge
module load anaconda/py3
source activate ipids

# Run experiment.
python security.py --num_cores 28
