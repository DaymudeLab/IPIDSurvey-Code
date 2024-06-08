/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PRNGShuffleIPID represents PRNG IPID selection using an iterated Knuth
// shuffle. Our implementation follows that of OpenBSD almost directly, but with
// added concurrency control.

#ifndef BENCHMARK_PRNGSHUFFLEIPID_H_
#define BENCHMARK_PRNGSHUFFLEIPID_H_

#include <mutex>
#include <random>
#include <vector>

#include "ipidmethod.h"

class PRNGShuffleIPID : IPIDMethod {
 public:
  // Constructs a PRNGShuffleIPID, initializing the shuffled permutation.
  PRNGShuffleIPID(uint32_t num_reserved);

  // Obtain the next IPID by returning the next value in the permutation and
  // inserting it in a randomly chosen location to reserve it.
  uint16_t get_ipid(Packet& pkt) override;

 protected:
  // The number of IPIDs to reserve.
  const uint16_t _kReservedIPIDs;

  // An index for the current head of the permutation.
  uint16_t _perm_head;

  // Random number generator and distribution.
  std::mt19937 _rng_mt;
  std::uniform_int_distribution<uint16_t> _swap_dist;

  // Lock for the permutation.
  std::mutex _lock;

  // The random permutation.
  std::vector<uint16_t> _perm;
};

#endif  // BENCHMARK_PRNGSHUFFLEIPID_H_
