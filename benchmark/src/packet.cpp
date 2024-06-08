/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "packet.h"

Packet::Packet(std::string src_addr, std::string dst_addr, std::string src_port,
               std::string dst_port, std::string protocol) {
  // Get numerical representations of source/destination IPv4 addresses.
  struct in_addr addr;
  inet_aton(src_addr.c_str(), &addr);
  _src_addr = addr.s_addr;
  inet_aton(dst_addr.c_str(), &addr);
  _dst_addr = addr.s_addr;

  // Get numerical representations of ports and protocol number.
  _src_port = std::stoi(src_port);
  _dst_port = std::stoi(dst_port);
  _protocol = std::stoi(protocol);
}
