#ifndef __KERNEL_BITMAP_H
#define __KERNEL_BITMAP_H
#include "global.h"
#define BITMAP_MASK 1

struct bitmap {
  uint32_t bmap_bytes_cnt;
  uint8_t* bits;
};

void bitmap_init(struct bitmap* bmap);
bool bitmap_scan_test(struct bitmap* bmap, uint32_t bit_idx);
int32_t bitmap_scan(struct bitmap* bmap, uint32_t cnt);
void bitmap_set(struct bitmap* bmap, uint32_t bit_idx, int8_t value);
#endif