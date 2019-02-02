#include "memory.h"
#include "debug.h"
#include "print.h"
#include "string.h"

#define PAGE_SZIE 4096
#define MEM_BITMAP_BASE 0xc009a000
#define K_HEAP_START 0xc0100000
#define TOTAL_MEMORY_BYTES_ADDR ((uint32_t*)0xb00)

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool {
  struct bitmap pool_bitmap;
  uint32_t phy_addr_start;
  uint32_t pool_size;
};

struct virtual_addr {
  struct bitmap vaddr_btmp;
  uint32_t vaddr_start;
};

struct pool kernel_pool, user_pool;

struct virtual_addr kernel_vaddr;

static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
  int32_t vaddr_start = 0, bit_idx_start = -1;
  uint32_t cnt = 0;
  if (pf == PF_KERNEL) {
    bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_btmp, pg_cnt);
    if (bit_idx_start == -1) return NULL;
    while (cnt < pg_cnt) {
      bitmap_set(&kernel_vaddr.vaddr_btmp, bit_idx_start + cnt, 1);
      cnt++;
    }
    vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PAGE_SZIE;
  } else {
    // 实现用户进程再补充
  }

  return (void*)vaddr_start;
}

static void mem_pool_init(uint32_t all_mem) {
  put_str(" mem pool init start\n");
  uint32_t page_table_size = 256 * PAGE_SZIE;
  uint32_t used_mem = page_table_size + 0x100000;
  uint32_t free_mem = all_mem - used_mem;
  uint32_t all_free_pages = free_mem / PAGE_SZIE;

  uint32_t kernel_free_pages = all_free_pages / 2;
  uint32_t user_free_pages = all_free_pages - kernel_free_pages;

  // 简化处理了,实现会有内存丢失
  uint32_t kbm_length = kernel_free_pages / 8;
  uint32_t ubm_length = user_free_pages / 8;

  uint32_t kp_st = used_mem;
  uint32_t up_st = kp_st + kernel_free_pages * PAGE_SZIE;

  kernel_pool.phy_addr_start = kp_st;
  kernel_pool.pool_size = kernel_free_pages * PAGE_SZIE;
  kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
  kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;

  user_pool.phy_addr_start = up_st;
  user_pool.pool_size = user_free_pages * PAGE_SZIE;
  user_pool.pool_bitmap.btmp_bytes_len = ubm_length;
  user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

  put_str(" kernel_pool_bitmap_start:");
  put_int((int)kernel_pool.pool_bitmap.bits);
  put_str(" kernel_pool_phy_addr_start:");
  put_int(kernel_pool.phy_addr_start);
  put_str("\n");

  put_str(" user_pool_bitmap_start:");
  put_int((int)user_pool.pool_bitmap.bits);
  put_str(" user_pool_phy_addr_start:");
  put_int(user_pool.phy_addr_start);
  put_str("\n");

  bitmap_init(&kernel_pool.pool_bitmap);
  bitmap_init(&user_pool.pool_bitmap);

  kernel_vaddr.vaddr_btmp.btmp_bytes_len = kbm_length;
  kernel_vaddr.vaddr_btmp.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
  kernel_vaddr.vaddr_start = K_HEAP_START;

  bitmap_init(&kernel_vaddr.vaddr_btmp);
  put_str(" mem pool init done\n");
}

// 分配一页物理内存
static void* palloc(struct pool* m_pool) {
  int32_t bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
  if (bit_idx == -1) return NULL;
  bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
  return (void*)(m_pool->phy_addr_start + bit_idx * PAGE_SZIE);
}

uint32_t* pte_ptr(uint32_t addr) {
  return (uint32_t*)(0xffc00000 + ((addr & 0xffc00000) >> 10) +
                     PTE_IDX(addr) * 4);
}

uint32_t* pde_ptr(uint32_t addr) {
  return (uint32_t*)(0xfffff000 + PDE_IDX(addr) * 4);
}

static void page_table_add(void* vaddr, void* page_phyaddr) {
  uint32_t __vaddr = (uint32_t)vaddr;
  uint32_t __page_phyaddr = (uint32_t)page_phyaddr;
  uint32_t* pde = pde_ptr(__vaddr);
  uint32_t* pte = pte_ptr(__vaddr);
  if (*pde & 0x00000001) {
    ASSERT(!(*pte & 0x00000001));
    if (!(*pte & 0x00000001)) {
      *pte = __page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
    } else {
      PANIC("pte repeat");
    }
  } else {
    uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
    *pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
    memset((void*)((int32_t)pte & 0xfffff000), 0, PAGE_SZIE);
    ASSERT(!(*pte & 0x00000001));
    *pte = __page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
  }
}

void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
  ASSERT(pg_cnt > 0 && pg_cnt < 3840);
  void* vaddr_start = vaddr_get(pf, pg_cnt);
  if (vaddr_start == NULL) return NULL;
  struct pool* m_pool = pf == PF_KERNEL ? &kernel_pool : &user_pool;
  uint32_t cnt = pg_cnt;
  uint32_t vddr = (uint32_t)vaddr_start;
  while (cnt-- > 0) {
    void* page_phyaddr = palloc(m_pool);
    if (page_phyaddr == NULL) return NULL;
    // 暂时没有回滚内存;
    page_table_add((void*)vddr, page_phyaddr);
    vddr += PAGE_SZIE;
  }
  return vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt) {
  void* vaddr_start = malloc_page(PF_KERNEL, pg_cnt);
  if (vaddr_start == NULL) return NULL;
  memset(vaddr_start, 0, PAGE_SZIE * pg_cnt);
  return vaddr_start;
}

void mem_init() {
  put_str("mem init start\n");
  uint32_t total_mem_bytes = *TOTAL_MEMORY_BYTES_ADDR;
  mem_pool_init(total_mem_bytes);
  put_str("mem init doen\n");
}
