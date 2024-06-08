/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "perbucketmipid.h"

PerBucketMIPID::PerBucketMIPID(uint32_t num_buckets)
    : _kNumBuckets(num_buckets),
      _rng_mt((std::random_device())()) {
  // Generate Siphash keys.
  std::uniform_int_distribution<uint64_t> _dist(
    0, std::numeric_limits<uint64_t>::max());
  _sipkey1 = _dist(_rng_mt);
  _sipkey2 = _dist(_rng_mt);

  // Initialize the bucket locks, requiring a dumb workaround because std::mutex
  // is not a movable type.
  std::vector<std::mutex> temp(num_buckets);
  _locks.swap(temp);

  // Initialize all bucket counters to zero.
  _counters.resize(num_buckets, 0);

  // Initialize all bucket last access times as the current time.
  auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  _times.resize(num_buckets, ms_since_epoch);
}

uint16_t PerBucketMIPID::get_ipid(Packet& pkt) {
  // Locate and lock the bucket.
  size_t idx = siphash3u32(pkt._dst_addr, pkt._src_addr, pkt._protocol,
                           _sipkey1, _sipkey2) % _kNumBuckets;
  _locks[idx].lock();

  // Get the bucket's last access time and the current time.
  uint64_t last = _times[idx];
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  _times[idx] = now;

  // Increment the bucket counter by a value chosen uniformly at random from 1
  // to the number of elapsed system ticks (milliseconds).
  uint64_t elapsed_ms = std::max(static_cast<uint64_t>(1), now - last);
  std::uniform_int_distribution<uint16_t> _dist(1, elapsed_ms);
  _counters[idx] += _dist(_rng_mt);
  uint16_t ipid = _counters[idx];

  // Release the bucket.
  _locks[idx].unlock();

  return ipid;
}
