/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "prngpureipid.h"

PRNGPureIPID::PRNGPureIPID(uint32_t num_threads)
    : _kSalt(0x73616C7470657061) {
  // Instantiate one RNG and IPID distribution per thread.
  std::random_device rd;
  for (auto i = 0; i < num_threads; i++) {
    _rng_mts.push_back(std::mt19937(rd()));
    _ipid_dists.push_back(std::uniform_int_distribution<uint16_t>(0, 65535));
  }
}

uint16_t PRNGPureIPID::get_ipid(Packet& pkt, uint32_t thread_id) {
  // Collapse the 64-bit salt value into a 16-bit one, following XNU.
  uint16_t salt16 = static_cast<uint16_t>(
      ((_kSalt >> 48) ^ (_kSalt >> 32) ^ (_kSalt >> 16) ^ _kSalt) & 0xFF);

  // Generate a random IPID that is not the 16-bit salt.
  uint16_t ipid;
  do {
    ipid = _ipid_dists[thread_id](_rng_mts[thread_id]);
  } while (ipid == salt16);

  return ipid ^ salt16;
}
