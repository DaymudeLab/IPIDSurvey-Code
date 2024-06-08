/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PerBucketMIPID represents per-bucket IPID selection where access to a bucket
// counter and its timers is protected by a lock. This differs from Linux's
// actual implementation, which just treats each individual bucket operation as
// atomic; see PerBucketLIPID. Buckets are located using Siphash and counters
// are incremented by a value chosen uniformly at random from 1 to the number of
// system ticks elapsed since this bucket was last accessed. In Linux, ticks are
// measured in "jiffies". For simplicity, we use milliseconds here.

#ifndef BENCHMARK_PERBUCKETMIPID_H_
#define BENCHMARK_PERBUCKETMIPID_H_

#include <chrono>
#include <limits>
#include <mutex>
#include <random>
#include <vector>

#include "ipidmethod.h"
#include "siphash.h"

class PerBucketMIPID : IPIDMethod {
 public:
  // Constructs a PerBucketMIPID, initializing the specified number of buckets.
  PerBucketMIPID(uint32_t num_buckets);

  // Obtain the next IPID by first locating a bucket by hashing the packet's
  // source/destination IP addresses, protocol number, and secret keys and then
  // incrementing that bucket counter by a value chosen uniformly at random from
  // 1 to the number of elapsed system ticks (milliseconds) since last access.
  uint16_t get_ipid(Packet& pkt) override;

 protected:
  // The fixed number of buckets.
  const uint32_t _kNumBuckets;

  // Random number generator for Siphash keys and stochastic increments.
  std::mt19937 _rng_mt;

  // Siphash keys.
  uint64_t _sipkey1, _sipkey2;

  // Per-bucket locks, counters, and last access times (in milliseconds).
  std::vector<std::mutex> _locks;
  std::vector<uint16_t> _counters;
  std::vector<uint64_t> _times;
};

#endif  // BENCHMARK_PERBUCKETMIPID_H_
