/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PerDestIPID represents per-destination IPID selection. In Windows, this is
// simply a hash table of IPID counters keyed by source/destination IP address
// pairs. When the table reaches a maximum size, a purge sequence is initiated
// that deletes any "stale" entries to free up space. In our case, we are
// accessing all counters so fast that the typical version of "staleness" is
// irrelevant; instead, we delete entries randomly in our purge sequences.

#ifndef BENCHMARK_PERDESTIPID_H_
#define BENCHMARK_PERDESTIPID_H_

#include <mutex>
#include <random>
#include <unordered_map>

#include "ipidmethod.h"

class PerDestIPID : IPIDMethod {
 public:
  // Constructs a PerDestIPID, initializing the hash table's maximum size and
  // entry purge probability.
  PerDestIPID(uint32_t max_dests, float purge_prob);

  // Obtain the next IPID by hashing the connection tuple and incrementing the
  // CPU counter.
  uint16_t get_ipid(Packet& pkt) override;

 protected:
  // Maximum capacity of destinations before a purge sequence is triggered.
  const uint32_t _kMaxDests;

  // Probability of deleting any one particular entry during a purge sequence.
  const float _kPurgeProb;

  // Random number generator and distributions.
  std::mt19937 _rng_mt;
  std::uniform_real_distribution<float> _purge_dist;
  std::uniform_int_distribution<uint16_t> _ipid_dist;

  // Lock for the hash table.
  std::mutex _lock;

  // Hash table mapping source/destination IP address pairs (concatenated as
  // one 64-bit integer) to IPID counters.
  std::unordered_map<uint64_t, uint16_t> _htable;
};

#endif  // BENCHMARK_PERDESTIPID_H_
