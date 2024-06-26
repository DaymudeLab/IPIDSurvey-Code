/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "perbucketlipid.h"

PerBucketLIPID::PerBucketLIPID(uint32_t num_buckets)
    : _kNumBuckets(num_buckets),
      _rng_mt((std::random_device())()) {
  // Generate Siphash keys.
  std::uniform_int_distribution<uint64_t> _dist(
    0, std::numeric_limits<uint64_t>::max());
  _sipkey1 = _dist(_rng_mt);
  _sipkey2 = _dist(_rng_mt);

  // Initialize all bucket counters to zero, requiring a dumb workaround because
  // std::atomic is not a movable type.
  std::vector<std::atomic<uint16_t>> temp1(num_buckets);
  _counters.swap(temp1);

  // Initialize all bucket last access times as the current time, again using
  // the workaround.
  std::vector<std::atomic<uint64_t>> temp2(num_buckets);
  _times.swap(temp2);
  auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  for (auto& t : _times) {
    t.store(ms_since_epoch, std::memory_order_relaxed);
  }
}

uint16_t PerBucketLIPID::get_ipid(Packet& pkt, uint32_t thread_id) {
  // Locate the bucket.
  size_t idx = siphash3u32(pkt._dst_addr, pkt._src_addr, pkt._protocol,
                           _sipkey1, _sipkey2) % _kNumBuckets;

  // Atomically get the bucket's last access time and swap in the current time.
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  uint64_t last = _times[idx].exchange(now, std::memory_order_relaxed);

  // Atomically increment the bucket counter by a value chosen uniformly at
  // random from 1 to the number of elapsed system ticks (milliseconds).
  uint64_t elapsed_ms = std::max(static_cast<uint64_t>(1), now - last);
  std::uniform_int_distribution<uint16_t> _dist(1, elapsed_ms);
  uint16_t inc = _dist(_rng_mt);
  uint16_t prev_ipid = _counters[idx].fetch_add(inc, std::memory_order_relaxed);

  return prev_ipid + inc;
}
