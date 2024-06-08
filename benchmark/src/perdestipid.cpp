/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "perdestipid.h"

PerDestIPID::PerDestIPID(uint32_t max_dests, float purge_prob)
    : _kMaxDests(max_dests),
      _kPurgeProb(purge_prob),
      _rng_mt((std::random_device())()),
      _purge_dist(std::uniform_real_distribution<float>(0.0, 1.0)),
      _ipid_dist(std::uniform_int_distribution<uint16_t>(0, 65535)) {
}

uint16_t PerDestIPID::get_ipid(Packet& pkt) {
  // Obtain exclusive access.
  _lock.lock();

  // If the hash table has reached capacity, initiate a purge sequence.
  if (_htable.size() >= _kMaxDests) {
    do {
      for (auto it = _htable.begin(); it != _htable.end(); ) {
        if (_purge_dist(_rng_mt) < _kPurgeProb) {
          it = _htable.erase(it);
        } else {
          ++it;
        }
      }
    } while (_htable.size() >= _kMaxDests);  // Just in case nothing purged.
  }

  // If this packet's source/destination IP address pair is not yet in the hash
  // table, add it as a random IPID; otherwise, increment the existing counter.
  uint16_t ipid = 0;
  uint64_t addr_p = static_cast<uint64_t>(pkt._src_addr) << 32 | pkt._dst_addr;
  if (!_htable.contains(addr_p)) {
    _htable[addr_p] = _ipid_dist(_rng_mt);
    ipid = 0;
  } else {
    _htable[addr_p]++;
    ipid = _htable[addr_p];
  }

  // Release exclusive access.
  _lock.unlock();

  return ipid;
}
