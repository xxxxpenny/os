#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "bitmap.h"
#include "stdint.h"

enum pool_flags { PF_KERNEL = 1, PF_USER = 2 };

#define PG_P_0 0
#define PG_P_1 1
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

void mem_init();
void* get_kernel_pages(uint32_t pg_cnt);
// extern struct pool kernel_pool, used_pool;
#endif