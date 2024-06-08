/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// Packet is a data class storing a packet's source/destination IPv4 addresses,
// source/destination ports, and protocol number.

#ifndef BENCHMARK_PACKET_H_
#define BENCHMARK_PACKET_H_

#include <arpa/inet.h>
#include <string>

class Packet {
 public:
  // Construct a new packet from strings representing source/destination IPv4
  // addresses, source/destination ports, and a protocol number.
  Packet(std::string src_addr, std::string dst_addr, std::string src_port,
         std::string dst_port, std::string protocol);

  // Data members.
  uint32_t _src_addr;
  uint32_t _dst_addr;
  uint32_t _src_port;
  uint32_t _dst_port;
  uint32_t _protocol;
};

#endif  // BENCHMARK_PACKET_H_
