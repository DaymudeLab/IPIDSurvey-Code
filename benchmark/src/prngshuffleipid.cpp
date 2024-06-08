/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "prngshuffleipid.h"

PRNGShuffleIPID::PRNGShuffleIPID(uint32_t num_reserved)
    : _kReservedIPIDs(static_cast<uint16_t>(num_reserved)),
      _perm_head(0),
      _rng_mt((std::random_device())()),
      _swap_dist(std::uniform_int_distribution<uint16_t>(0,_kReservedIPIDs-1)) {
  // Generate the initial sequential permutation of [0, ..., 2^16 - 1].
  _perm.resize(65536, 0);
  std::iota(_perm.begin(), _perm.end(), 0);

  // Shuffle the permutation.
  for (size_t i = 0; i < _perm.size(); i++) {
    std::uniform_int_distribution<size_t> _dist(0, i);
    size_t i2 = _dist(_rng_mt);
    _perm[i] = _perm[i2];
    _perm[i2] = i;
  }
}

uint16_t PRNGShuffleIPID::get_ipid(Packet& pkt) {
  // Obtain exclusive access.
  _lock.lock();

  // Get the next IPID in the permutation and then shuffle it back in.
  uint16_t ipid = 0;
  do {
    // Generate a random swap index between the IPID's current position and K
    // previous positions, taking into consideration the cyclic permutation.
    // Note that this arithmetic works because _perm_head is an unsigned 16-bit
    // integer. If it were anything else, we'd need to be more careful.
    uint16_t i2 = _perm_head - _swap_dist(_rng_mt);

    // Swap the IPIDs and advance the permutation head.
    ipid = _perm[_perm_head];
    _perm[_perm_head] = _perm[i2];
    _perm[i2] = ipid;
    _perm_head = (_perm_head + 1) % _perm.size();
  } while (ipid == 0);

  // Release exclusive access.
  _lock.unlock();

  return ipid;
}
