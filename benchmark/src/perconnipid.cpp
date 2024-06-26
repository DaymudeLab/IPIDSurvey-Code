/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "perconnipid.h"

PerConnIPID::PerConnIPID() {}

uint16_t PerConnIPID::get_ipid(Packet& pkt, uint32_t thread_id) {
  uint16_t ipid = 0;
  return ipid + 1;
}
