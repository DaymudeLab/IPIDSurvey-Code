/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "perbucketshuffleipid.h"

PerBucketShuffleIPID::PerBucketShuffleIPID(uint32_t num_buckets)
    : _kNumBuckets(num_buckets),
      _kReservedIPIDs(32768) {
  // Set up random number generators and distributions.
  std::random_device rd;
  _rng_mts.resize(num_buckets, std::mt19937(rd()));
  _swap_dists.resize(num_buckets, std::uniform_int_distribution<uint16_t>(0,
      _kReservedIPIDs - 1));

  // Initialize the bucket locks, requiring a dumb workaround because std::mutex
  // is not a movable type.
  std::vector<std::mutex> temp(num_buckets);
  _locks.swap(temp);

  // Initialize the permutations.
  _perms.resize(num_buckets, std::vector<uint16_t>());
  for (auto& _perm : _perms) {
    // Generate the initial sequential permutation of [0, ..., 2^16 - 1].
    _perm.resize(65536, 0);
    std::iota(_perm.begin(), _perm.end(), 0);

    // Shuffle the permutation.
    for (size_t i = 0; i < _perm.size(); i++) {
      std::uniform_int_distribution<size_t> _dist(0, i);
      size_t i2 = _dist(_rng_mts[0]);
      _perm[i] = _perm[i2];
      _perm[i2] = i;
    }
  }

  // Initialize the permutation head indices.
  _perm_heads.resize(num_buckets, 0);

  // Generate Siphash keys.
  std::mt19937_64 rng((std::random_device())());
  std::uniform_int_distribution<uint64_t> _dist(
    0, std::numeric_limits<uint64_t>::max());
  _sipkey1 = _dist(rng);
  _sipkey2 = _dist(rng);
}

uint16_t PerBucketShuffleIPID::get_ipid(Packet& pkt) {
  // Locate and lock the bucket.
  size_t idx = siphash3u32(pkt._dst_addr, pkt._src_addr, pkt._protocol,
                           _sipkey1, _sipkey2) % _kNumBuckets;
  _locks[idx].lock();

  // Get the next IPID in the bucket permutation and then shuffle it back in.
  uint16_t ipid = 0;
  do {
    // Generate a random swap index between the IPID's current position and K
    // previous positions, taking into consideration the cyclic permutation.
    // Note that this arithmetic works because _perm_head is an unsigned 16-bit
    // integer. If it were anything else, we'd need to be more careful.
    uint16_t i2 = _perm_heads[idx] - _swap_dists[idx](_rng_mts[idx]);

    // Swap the IPIDs and advance the permutation head.
    ipid = _perms[idx][_perm_heads[idx]];
    _perms[idx][_perm_heads[idx]] = _perms[idx][i2];
    _perms[idx][i2] = ipid;
    _perm_heads[idx] = (_perm_heads[idx] + 1) % _perms[idx].size();
  } while (ipid == 0);

  // Release the bucket.
  _locks[idx].unlock();

  return ipid;
}
