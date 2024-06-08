/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PerConnIPID represents per-connection IPID selection. In practice, Linux
// handles TCP traffic in sockets, and sockets are each assigned their own
// counters. Any time a thread is setting up a packet as part of a connection,
// it's reponding to a system call that already provided the socket structure
// as context. So there's no cost to "locating" the connection counter, like
// there is in per-bucket. Also, according to Jed, there's essentially no case
// where multiple cores would be contending over the same socket. So from our
// performance perspective, getting a per-connection IPID is as simple as
// standing up a uint16_t and incrementing it.

#ifndef BENCHMARK_PERCONNIPID_H_
#define BENCHMARK_PERCONNIPID_H_

#include "ipidmethod.h"
#include "siphash.h"

class PerConnIPID : IPIDMethod {
 public:
  // Constructs a PerConnIPID without any internal data.
  PerConnIPID();

  // Obtain the "next IPID" by faking an increment.
  uint16_t get_ipid(Packet& pkt) override;
};

#endif  // BENCHMARK_PERCONNIPID_H_
