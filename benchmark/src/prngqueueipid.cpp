/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "prngqueueipid.h"

PRNGQueueIPID::PRNGQueueIPID(uint32_t queue_size)
    : _kQueueSize(queue_size),
      _num_queued(0),
      _queue_head(0),
      _rng_mt((std::random_device())()),
      _ipid_dist(std::uniform_int_distribution<uint16_t>(0, 65535)),
      _queue(std::vector<uint16_t>(queue_size, 0)),
      _lookup(std::vector<bool>(65536, 0)) {}

uint16_t PRNGQueueIPID::get_ipid(Packet& pkt) {
  // Obtain exclusive access.
  _lock.lock();

  // Generate a random IPID that is not queued or zero.
  uint16_t ipid;
  do {
    ipid = _ipid_dist(_rng_mt);
  } while (_lookup[ipid] || ipid == 0);

  // Insert this IPID into the queue and, if the queue is at capacity, dequeue
  // the oldest element.
  if (_num_queued < _kQueueSize) {
    _queue[_num_queued] = ipid;
    _lookup[ipid] = true;
    _num_queued++;
  } else {
    uint16_t other = _queue[_queue_head];
    _queue[_queue_head] = ipid;
    _queue_head = (_queue_head + 1) % _kQueueSize;
    _lookup[ipid] = true;
    _lookup[other] = false;
  }

  // Release exclusive access.
  _lock.unlock();

  return ipid;
}
