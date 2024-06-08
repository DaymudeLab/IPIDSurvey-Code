/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "globalipid.h"

GlobalIPID::GlobalIPID()
    : _counter(0) {}

uint16_t GlobalIPID::get_ipid(Packet& pkt) {
  uint16_t temp = _counter.fetch_add(1, std::memory_order_relaxed);
  return temp + 1;
}
