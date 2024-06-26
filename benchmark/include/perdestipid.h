/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PerDestIPID represents per-destination IPID selection. In Windows, this is
// a hash table ("PathSet") of (IPID counter, last access timestamp) pairs keyed
// by source/destination IP address pairs. Every 0.5 seconds, the PathSet's size
// is checked. If the size exceeds its "purge threshold" or >= 5000 entries were
// added since the last check, a purge sequence is initiated that deletes up to
// max(1000, # entries added since last check) "stale" entries to free up space.
// There are two cases for what's considered "stale": if the PathSet's size is
// 1-2x its purge threshold, then entries accessed longer than 60 s ago are
// considered stale; if the PathSet's size exceeds 2x its purge threshold, all
// entries are considered stale.

#ifndef BENCHMARK_PERDESTIPID_H_
#define BENCHMARK_PERDESTIPID_H_

#include <mutex>
#include <random>
#include <unordered_map>
#include <utility>

#include "ipidmethod.h"

class PerDestIPID : IPIDMethod {
 public:
  // Constructs a PerDestIPID, initializing the hash table's purge threshold.
  PerDestIPID(uint32_t purge_threshold);

  // Obtain the next IPID by initializing or incrementing the corresponding
  // destination counter and initiating a purge sequence if it is time.
  uint16_t get_ipid(Packet& pkt, uint32_t thread_id) override;

 protected:
  // Threshold of destination counters over which a purge sequence is triggered.
  const uint32_t _kPurgeThreshold;

  // Random number generator and distributions.
  std::mt19937 _rng_mt;
  std::uniform_int_distribution<uint16_t> _ipid_dist;

  // Lock for the hash table.
  std::mutex _lock;

  // Hash table ("PathSet") mapping src/dst IP address pairs (concatenated as
  // one 64-bit integer) to (IPID counter, last access timestamp) pairs.
  std::unordered_map<uint64_t, std::pair<uint16_t,
      std::chrono::time_point<std::chrono::steady_clock>>> _pathset;

  // Timestamp of last purge check and number of entries added since last check.
  std::chrono::time_point<std::chrono::steady_clock> _tpurge;
  uint32_t _num_added_since_check;
};

#endif  // BENCHMARK_PERDESTIPID_H_
