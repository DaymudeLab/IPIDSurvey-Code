/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// PRNGQueueIPID represents PRNG IPID selection using a searchable queue. Our
// implementation follows those of FreeBSD and XNU almost directly, but with
// concurrency handled correctly.

#ifndef BENCHMARK_PRNGQUEUEIPID_H_
#define BENCHMARK_PRNGQUEUEIPID_H_

#include <mutex>
#include <random>
#include <vector>

#include "ipidmethod.h"

class PRNGQueueIPID : IPIDMethod {
 public:
  // Constructs a PRNGQueueIPID, initializing the searchable queue.
  PRNGQueueIPID(uint32_t queue_size);

  // Obtain the next IPID by generating an IPID that is not currently in the
  // queue and then inserting it into the queue.
  uint16_t get_ipid(Packet& pkt, uint32_t thread_id) override;

 protected:
  // The size of the searchable queue; i.e., the number of IPIDs to reserve.
  const uint32_t _kQueueSize;

  // The current number of IPIDs in the searchable queue.
  uint32_t _num_queued;

  // An index for the current head of the searchable queue.
  uint32_t _queue_head;

  // Random number generator and distribution.
  std::mt19937 _rng_mt;
  std::uniform_int_distribution<uint16_t> _ipid_dist;

  // Lock for the searchable queue.
  std::mutex _lock;

  // The "searchable queue": a vector of IPIDs currently in the queue, and a bit
  // array of length 2^16 whose i-th entry is 1 iff IPID i is currently queued.
  std::vector<uint16_t> _queue;
  std::vector<bool> _lookup;
};

#endif  // BENCHMARK_PRNGQUEUEIPID_H_
