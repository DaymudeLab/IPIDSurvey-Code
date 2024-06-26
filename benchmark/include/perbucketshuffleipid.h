/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PerBucketShuffleIPID represents a newly proposed algorithm that combines
// per-bucket IPID selection with PRNG IPID selection using an iterated Knuth
// shuffle. Each bucket has its own random permutation of IPIDs protected by a
// lock. Buckets are located using Siphash as in Linux's per-bucket selection.

#ifndef BENCHMARK_PERBUCKETSHUFFLEIPID_H_
#define BENCHMARK_PERBUCKETSHUFFLEIPID_H_

#include <limits>
#include <mutex>
#include <random>
#include <vector>

#include "ipidmethod.h"
#include "siphash.h"

class PerBucketShuffleIPID : IPIDMethod {
 public:
  // Constructs a PerBucketShuffleIPID, instantiating bucket PRNGs, locks, and
  // IPID permutations.
  PerBucketShuffleIPID(uint32_t num_buckets);

  // Obtain the next IPID by first locating a bucket by hashing the packet's
  // source/destination IP addresses, protocol number, and secret keys and then
  // returning the next value in the bucket permutation and inserting it in a
  // randomly chosen location to reserve it.
  uint16_t get_ipid(Packet& pkt, uint32_t thread_id) override;

 protected:
  // The fixed number of buckets.
  const uint32_t _kNumBuckets;

  // The fixed number of IPIDs to reserve.
  const uint16_t _kReservedIPIDs;

  // Per-bucket random number generators, random number distributions, locks,
  // permutations, and indices.
  std::vector<std::mt19937> _rng_mts;
  std::vector<std::uniform_int_distribution<uint16_t>> _swap_dists;
  std::vector<std::mutex> _locks;
  std::vector<std::vector<uint16_t>> _perms;
  std::vector<uint16_t> _perm_heads;

  // Siphash keys.
  uint64_t _sipkey1, _sipkey2;
};

#endif  // BENCHMARK_PERBUCKETSHUFFLEIPID_H_
