/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

// Functions for computing Siphash. Our Implementation follows Linux's and that
// of Jean-Philippe Aumasson: https://github.com/veorq/SipHash.

#ifndef BENCHMARK_SIPHASH_H_
#define BENCHMARK_SIPHASH_H_

#include <cstdint>

// Computes Siphash for three uint32_t values. Based on Linux's implementation:
// https://elixir.bootlin.com/linux/v6.9/source/lib/siphash.c#L215
uint64_t siphash3u32(uint32_t val1, uint32_t val2, uint32_t val3, uint64_t key1,
                     uint64_t key2);

#endif  // BENCHMARK_SIPHASH_H_
