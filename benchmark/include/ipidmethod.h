/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// IPIDMethod is an abstract class representing an IPID selection method. Child
// classes implementing a specific selection method can override this class. We
// need such a class so that we have an abstract type to point at when setting
// up a user-chosen IPID selection method.

#ifndef BENCHMARK_IPIDMETHOD_H_
#define BENCHMARK_IPIDMETHOD_H_

#include <cstdint>

#include "packet.h"

class IPIDMethod {
 public:
  // Pure virtual function to obtain this method's next IPID as a function of
  // the packet header information.
  virtual uint16_t get_ipid(Packet& pkt, uint32_t thread_id) = 0;
};

#endif  // BENCHMARK_IPIDMETHOD_H_
