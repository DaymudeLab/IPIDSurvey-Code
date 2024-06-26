/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "perdestipid.h"

PerDestIPID::PerDestIPID(uint32_t purge_threshold)
    : _kPurgeThreshold(purge_threshold),
      _rng_mt((std::random_device())()),
      _ipid_dist(std::uniform_int_distribution<uint16_t>(0, 65535)),
      _tpurge(std::chrono::steady_clock::now()),
      _num_added_since_check(0) {
}

uint16_t PerDestIPID::get_ipid(Packet& pkt, uint32_t thread_id) {
  // Obtain exclusive access.
  _lock.lock();

  // If 0.5 s have elapsed since the last purge check, do a purge check.
  if (std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - _tpurge) >=
          std::chrono::milliseconds(500)) {
    // Initiate a purge sequence if the PathSet has grown too large or if
    // flooding is detected.
    if (_pathset.size() > _kPurgeThreshold || _num_added_since_check > 5000) {
      // Determine the maximum number of elements to remove.
      uint32_t to_purge = std::max(static_cast<uint32_t>(1000),
                                   _num_added_since_check);

      // Purge elements based on staleness condition.
      if (_pathset.size() > 2 * _kPurgeThreshold) {  // All are stale.
        for (auto it = _pathset.begin(); it != _pathset.end() && to_purge > 0;) {
          it = _pathset.erase(it);
          to_purge--;
        }
      } else if (_pathset.size() > _kPurgeThreshold) {  // > 60 s is stale.
        for (auto it = _pathset.begin(); it != _pathset.end() && to_purge > 0;) {
          if (std::chrono::duration_cast<std::chrono::seconds>(
                  std::chrono::steady_clock::now() - it->second.second) >=
                  std::chrono::seconds(60)) {
            it = _pathset.erase(it);
            to_purge--;
          } else {
            ++it;
          }
        }
      }

      // Reset the purge timestep and number of entries added since last check.
      _tpurge = std::chrono::steady_clock::now();
      _num_added_since_check = 0;
    }
  }

  // If this packet's source/destination IP address pair is not yet in the hash
  // table, add it as a random IPID; otherwise, increment the existing counter.
  uint16_t ipid = 0;
  uint64_t addr_p = static_cast<uint64_t>(pkt._src_addr) << 32 | pkt._dst_addr;
  if (!_pathset.contains(addr_p)) {
    _pathset[addr_p] = std::make_pair(_ipid_dist(_rng_mt),
                                      std::chrono::steady_clock::now());
  } else {
    _pathset[addr_p].first++;
    _pathset[addr_p].second = std::chrono::steady_clock::now();
  }
  ipid = _pathset[addr_p].first;

  // Release exclusive access.
  _lock.unlock();

  return ipid;
}
