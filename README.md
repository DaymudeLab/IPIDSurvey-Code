# A Comparative Analysis of IPID Selection Methods

This repository contains the supporting simulation, benchmarking, and plotting code for the paper **A Taxonomy and Comparative Analysis of IPv4 ID Selection Correctness, Security, and Performance** by Joshua J. Daymude, Antonio M. Espinoza, Sean Bergen, Benjamin Mixon&ndash;Baca, Jeffrey Knockel, and Jedidiah R. Crandall.


## Getting Started

Before running any scripts, please build and activate the necessary `conda` environment by running:
```
conda create --file requirements.yml
conda activate IPIDSurvey
```

Many of our results involve computationally expensive sampling or benchmarking experiments.
If you would like to reproduce our results from scratch, follow the instructions below.
Otherwise, download our pre-computed results (available [here](https://drive.google.com/drive/folders/1zvz4_qmtU2vlOMPNkFbv6G5jFD0zmUIJ?usp=sharing)) and extract them in this directory as `results/`.
You can then call the corresponding plotting scripts in `collisions.py` (correctness), `security.py` (security), and `benchplot.py` (performance) without having to wait for the long runtimes.


## Usage Guide

### Correctness

`collisions.py` contains our correctness analysis (Section 4.2).
Run it with `python collisions.py -P <num_cores>`, where `-P` optionally specifies additional cores to speed up the calculations depending on sampling.
Use `python collisions.py --help` for all options.
If `results/collisions/` already exists, it will use the results therein instead of calculating them from scratch.


### Security

`security.py` contains our security analysis (Section 4.3).
Run it with `python security.py -P <num_cores>`, where `-P` optionally specifies additional cores to speed up the calculations depending on sampling.
Use `python security.py --help` for all options.
If `results/security/` already exists, it will use the results therein instead of calculating them from scratch.


### Performance

Our runtime benchmark (Section 4.4) is implemented in C++ and was run on a 128-core machine (dual-socket 2x 64-core AMD EPYC 7713 Zen3).
If you downloaded our pre-computed results (in this case, `results/benchmark/`), then you can plot the outcome with `python benchplot.py`.

If you are trying to run the benchmark from scratch, navigate to `benchmark/` and then build with `./build.sh`.
Any build errors likely will have to do with your C++ version (we require C++20), or missing `boost` libraries (we use `boost::program_options`).
A successful build will create a `build/` directory containing a variety of build artifacts.

View the executable's options using `./build/benchmark --help`.
Our benchmark can be replicated using `./sol_bench.sh`, though you may need to adjust the parameters based on your hardware.
