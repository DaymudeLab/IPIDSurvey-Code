#!/bin/bash

#SBATCH -N 1                          # Number of nodes.
#SBATCH -c 128                        # Number of cores per node.
#SBATCH -p general                    # Partition.
#SBATCH -t 2-00:00:00                 # Wall clock time for job (D-HH:MM:SS)
#SBATCH -o logs/%j.out                # STDOUT (%j = JOBID)
#SBATCH -e logs/%j.err                # STDERR (%j = JOBID)
#SBATCH --hint=nomultithread	      # Hopefully avoid multi/hyper threads.
#SBATCH --mail-type=BEGIN,FAIL,END    # Send start/stop/failure notifications.
#SBATCH --mail-user=%u@asu.edu        # Address to send notifications to.

# Run this script with:
# sbatch sol_bench.sh

# Experiment parameters held constant across IPID selection methods.
fname=packets.csv
trials=11
delta=10
warmup=100
cpunum=128

# Load a recent enough version of gcc/g++ that recognizes C++20.
# Note: comment this out if you're not using the ASU Sol supercomputer.
module load gcc-12.1.0-gcc-11.2.0
module load cmake-3.23.1-gcc-11.2.0

# Do a clean build of the benchmark.
rm -rf build/
./build.sh

function run_benchmark(){
    # Run experiment. (Use ./benchmark --help to see the options.)
    # Case 1: we have an ipid method that needs some -a argument for benchmark
    if (($# == 2)); then
	./build/benchmark -f $fname -m $1 -a $2 -t $trials -d $delta -w $warmup -c $cpunum
    # Case 2: we do not need the -a argument when running benchmark
    else
	./build/benchmark -f $fname -m $1 -t $trials -d $delta -w $warmup -c $cpunum
    fi
}

# EXPERIMENTS
run_benchmark global
run_benchmark perconn
run_benchmark perdest 32768          # Windows sets a purge threshold of 2^15.
run_benchmark perbucketl 2048        # 2^11 buckets (Linux minimum).
run_benchmark perbucketl 262144      # 2^18 buckets (Linux maximum).
run_benchmark perbucketm 2048        # 2^11 buckets (Linux minimum).
run_benchmark perbucketm 262144      # 2^18 buckets (Linux maximum).
run_benchmark prngqueue 4096         # macOS/XNU reserves 2^12 IPIDs.
run_benchmark prngqueue 8192         # FreeBSD reserves 2^13 IPIDs.
run_benchmark prngshuffle 32768      # OpenBSD reserves 2^15 IPIDs.
run_benchmark perbucketshuffle 12    # = in storage to 2^18 bucket counters.
