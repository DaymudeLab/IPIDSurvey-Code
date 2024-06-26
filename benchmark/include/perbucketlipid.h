/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PerBucketLIPID represents Linux's per-bucket IPID selection where access to a
// bucket's counter and timer are individually atomic, but not treated as a
// critical section altogether. This allows some minor race conditions avoided
// with bucket locks; see PerBucketMIPID. Buckets are located using Siphash and
// counters are incremented by a value chosen uniformly at random from 1 to the
// number of system ticks elapsed since this bucket was last accessed. In Linux,
// ticks are measured in "jiffies". For simplicity, we use milliseconds here.

#ifndef BENCHMARK_PERBUCKETLIPID_H_
#define BENCHMARK_PERBUCKETLIPID_H_

#include <atomic>
#include <chrono>
#include <limits>
#include <random>
#include <vector>

#include "ipidmethod.h"
#include "siphash.h"

class PerBucketLIPID : IPIDMethod {
 public:
  // Constructs a PerBucketLIPID, initializing the specified number of buckets.
  PerBucketLIPID(uint32_t num_buckets);

  // Obtain the next IPID by first locating a bucket by hashing the packet's
  // source/destination IP addresses, protocol number, and secret keys and then
  // incrementing that bucket counter by a value chosen uniformly at random from
  // 1 to the number of elapsed system ticks (milliseconds) since last access.
  uint16_t get_ipid(Packet& pkt, uint32_t thread_id) override;

 protected:
  // The fixed number of buckets.
  const uint32_t _kNumBuckets;

  // Random number generator for Siphash keys and stochastic increments.
  std::mt19937 _rng_mt;

  // Siphash keys.
  uint64_t _sipkey1, _sipkey2;

  // Per-bucket counters and last access times (in milliseconds).
  std::vector<std::atomic<uint16_t>> _counters;
  std::vector<std::atomic<uint64_t>> _times;
};

#endif  // BENCHMARK_PERBUCKETLIPID_H_
