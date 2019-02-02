#include "bitmap.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "stdint.h"
#include "string.h"

void bitmap_init(struct bitmap* bmap) {
  memset(bmap->bits, 0, bmap->btmp_bytes_len);
}

bool bitmap_scan_test(struct bitmap* bmap, uint32_t bit_idx) {
  uint32_t byte_idx = bit_idx / 8;
  uint32_t bit_odd = bit_idx % 8;
  return bmap->bits[byte_idx] & (BITMAP_MASK << bit_odd);
}

int32_t bitmap_scan(struct bitmap* bmap, uint32_t cnt) {
  uint32_t byte_idx = 0;
  while (bmap->bits[byte_idx] == 0xff && byte_idx < bmap->btmp_bytes_len) {
    byte_idx++;
  }
  ASSERT(byte_idx < bmap->btmp_bytes_len);
  if (byte_idx == bmap->btmp_bytes_len) {
    return -1;
  }

  uint32_t bit_odd = 0;
  while (bmap->bits[byte_idx] & (uint8_t)(BITMAP_MASK << bit_odd)) {
    bit_odd++;
  }
  uint32_t bit_idx_start = byte_idx * 8 + bit_odd;
  if (cnt == 1) {
    return bit_idx_start;
  }

  uint32_t bit_left = bmap->btmp_bytes_len * 8 - bit_idx_start;
  uint32_t next_bit = bit_idx_start + 1;
  uint32_t count = 1;

  bit_idx_start = -1;
  while (bit_left-- > 0) {
    if (!bitmap_scan_test(bmap, next_bit)) {
      count++;
    } else {
      count = 0;
    }

    if (count == cnt) {
      bit_idx_start = next_bit - cnt + 1;
      return bit_idx_start;
    }
    next_bit++;
  }
  return bit_idx_start;
}

void bitmap_set(struct bitmap* bmap, uint32_t bit_idx, int8_t value) {
  ASSERT(value == 0 || value == 1);
  uint32_t byte_idx = bit_idx / 8;
  uint32_t bit_odd = bit_idx % 8;

  if (value) {
    bmap->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
  } else {
    bmap->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
  }
}