#!/bin/bash

# Experiment parameters held constant across IPID selection methods.
fname=packets.csv
resultspath="../results/benchmark/local"
trials=11
delta=10
warmup=100
cpunum=128

# Do a clean build of the benchmark.
rm -rf build/
./build.sh

function run_benchmark(){
    # Run experiment. (Use ./benchmark --help to see the options.)
    # Case 1: we have an ipid method that needs some -a argument for benchmark
    if (($# == 2)); then
	./build/benchmark -f $fname -r $resultspath -m $1 -a $2 -t $trials -d $delta -w $warmup -c $cpunum
    # Case 2: we do not need the -a argument when running benchmark
    else
	./build/benchmark -f $fname -r $resultspath -m $1 -t $trials -d $delta -w $warmup -c $cpunum
    fi
}

# EXPERIMENTS
run_benchmark global                 # FreeBSD, default.
run_benchmark perconn                # Linux, for connection-bound packets.
run_benchmark perdest 32768          # Windows sets a purge threshold of 2^15.
run_benchmark perbucketl 2048        # 2^11 buckets (Linux minimum).
run_benchmark perbucketl 262144      # 2^18 buckets (Linux maximum).
run_benchmark perbucketm 2048        # 2^11 buckets (Linux minimum).
run_benchmark perbucketm 262144      # 2^18 buckets (Linux maximum).
run_benchmark prngqueue 8192         # FreeBSD reserves 2^13 IPIDs.
run_benchmark prngshuffle 32768      # OpenBSD reserves 2^15 IPIDs.
run_benchmark prngpure               # macOS/XNU.
