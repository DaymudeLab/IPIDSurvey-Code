/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PRNGPureIPID represents pure PRNG IPID selection. Our implementation follows
// that of macOS/XNU almost directly, but with added concurrency control.

#ifndef BENCHMARK_PRNGPUREIPID_H_
#define BENCHMARK_PRNGPUREIPID_H_

#include <random>
#include <vector>

#include "ipidmethod.h"

class PRNGPureIPID : IPIDMethod {
 public:
  // Constructs a PRNGPureIPID, initializing the PRNGs.
  PRNGPureIPID(uint32_t num_threads);

  // Obtain the next IPID by generating an IPID value uniformly at random and
  // then salting it, ensuring the final value is non-zero.
  uint16_t get_ipid(Packet& pkt, uint32_t thread_id) override;

 protected:
  // The salt value. In macOS/XNU, this is packet-specific, but for a benchmark
  // we can simply use a constant.
  const uint64_t _kSalt;

  // Random number generators and distributions for each thread.
  std::vector<std::mt19937> _rng_mts;
  std::vector<std::uniform_int_distribution<uint16_t>> _ipid_dists;
};

#endif  // BENCHMARK_PRNGPUREIPID_H_
