/* Copyright (C) 2024 Joshua J. Daymude, Antonio M. Espinoza, and Sean Bergen.
 * See the LICENSE file for the full MIT license and copyright notice. */

#include "siphash.h"

inline uint64_t rotl(uint64_t a, uint64_t b) {
  return static_cast<uint64_t>((a << b) | (a >> (64 - b)));
}

void sipround(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3) {
  v0 += v1;
  v1 = rotl(v1, 13);
  v1 ^= v0;
  v0 = rotl(v0, 32);
  v2 += v3;
  v3 = rotl(v3, 16);
  v3 ^= v2;
  v0 += v3;
  v3 = rotl(v3, 21);
  v3 ^= v0;
  v2 += v1;
  v1 = rotl(v1, 17);
  v1 ^= v2;
  v2 = rotl(v2, 32);
}

uint64_t siphash3u32(uint32_t val1, uint32_t val2, uint32_t val3, uint64_t key1,
                     uint64_t key2) {
  // Combine the first and second values.
  uint64_t combined = static_cast<uint64_t>(val2) << 32 | val1;

  // Preamble.
  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;
  uint64_t b = static_cast<uint64_t>(12) << 56;  // 12 = 3 values * 4 bytes.
  v3 ^= key2;
  v2 ^= key1;
  v1 ^= key2;
  v0 ^= key1;

  // Do the siprounds.
  v3 ^= combined;
  sipround(v0, v1, v2, v3);
  sipround(v0, v1, v2, v3);
  v0 ^= combined;
  b |= val3;

  // Postamble.
  v3 ^= b;
  sipround(v0, v1, v2, v3);
  sipround(v0, v1, v2, v3);
  v0 ^= b;
  v2 ^= 0xff;
  sipround(v0, v1, v2, v3);
  sipround(v0, v1, v2, v3);
  sipround(v0, v1, v2, v3);
  sipround(v0, v1, v2, v3);

  return (v0 ^ v1) ^ (v2 ^ v3);
}
