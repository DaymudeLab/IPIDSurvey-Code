/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// GlobalIPID represents globally incrementing IPID selection. All CPUs share a
// single, global atomic counter.

#ifndef BENCHMARK_GLOBALIPID_H_
#define BENCHMARK_GLOBALIPID_H_

#include <atomic>

#include "ipidmethod.h"

class GlobalIPID : IPIDMethod {
 public:
  // Constructs a GlobalIPID, initializing the global counter to zero.
  GlobalIPID();

  // Obtain the next IPID by incrementing the global counter.
  uint16_t get_ipid(Packet& pkt) override;

 protected:
  // Global counter.
  std::atomic<uint16_t> _counter;
};

#endif  // BENCHMARK_GLOBALIPID_H_
