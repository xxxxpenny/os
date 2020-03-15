#include "kernel/memory.h"
#include "kernel/debug.h"
#include "kernel/interrupt.h"
#include "lib/kernel/print.h"
#include "lib/string.h"
#include "thread/sync.h"
#include "thread/thread.h"

#define PAGE_SZIE 4096
#define MEM_BITMAP_BASE 0xc009a000
#define K_HEAP_START 0xc0100000
#define TOTAL_MEMORY_BYTES_ADDR ((uint32_t*)0xb00)

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool {
  struct bitmap pool_bitmap;
  struct lock lock;
  uint32_t phy_addr_start;
  uint32_t pool_size;
};

struct arena {
  struct mem_block_desc* desc;
  uint32_t cnt;
  bool large;
};

struct pool kernel_pool, user_pool;

struct virtual_addr kernel_vaddr;

struct mem_block_desc k_block_descs[DESC_CNT];

void block_desc_init(struct mem_block_desc* desc_array) {
  uint16_t idx = 0, blcok_size = 16;
  for (idx = 0; idx < DESC_CNT; idx++) {
    desc_array[idx].block_size = blcok_size;
    desc_array[idx].blocks_per_arnea =
        (PAGE_SIZE - sizeof(struct arena)) / blcok_size;
    list_init(&desc_array[idx].free_list);
    blcok_size = blcok_size * 2;
  }
}

static struct mem_block* arena2block(struct arena* a, uint32_t idx) {
  return (struct mem_block*)((uint32_t)a + sizeof(struct arena) +
                             idx * a->desc->block_size);
}

static struct arena* block2arena(struct mem_block* b) {
  return (struct arena*)((uint32_t)b & 0xfffff000);
}

void* sys_malloc(uint32_t size) {
  enum pool_flags pf;
  struct pool* mem_pool;
  uint32_t pool_size;
  struct mem_block_desc* block_descs;
  struct task_struct* cur = running_thread();
  if (cur->pgdir == NULL) {
    pf = PF_KERNEL;
    mem_pool = &kernel_pool;
    pool_size = kernel_pool.pool_size;
    block_descs = k_block_descs;
  } else {
    pf = PF_USER;
    mem_pool = &user_pool;
    pool_size = user_pool.pool_size;
    block_descs = cur->u_block_descs;
  }

  if (!(size > 0 && size < pool_size)) {
    return NULL;
  }

  struct arena* a;
  struct mem_block* b;
  lock_acquire(&mem_pool->lock);
  if (size > 1024) {
    uint32_t page_cnt = (size + sizeof(struct arena)) % PAGE_SIZE == 0
                            ? (size + sizeof(struct arena)) / PAGE_SIZE
                            : (size + sizeof(struct arena)) / PAGE_SIZE + 1;
    a = malloc_page(pf, page_cnt);
    if (a != NULL) {
      memset(a, 0, PAGE_SIZE * page_cnt);
      a->desc = NULL;
      a->cnt = page_cnt;
      a->large = true;
      lock_release(&mem_pool->lock);
      return (void*)(a + 1);
    } else {
      lock_release(&mem_pool->lock);
      ASSERT(NULL != NULL);
      return NULL;
    }

  } else {
    uint8_t desc_idx;
    for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
      if (size <= block_descs[desc_idx].block_size) {
        break;
      }
    }
    if (list_empty(&block_descs[desc_idx].free_list)) {
      a = malloc_page(pf, 1);
      if (a == NULL) {
        lock_release(&mem_pool->lock);
        ASSERT(NULL != NULL);
        return NULL;
      }
      memset(a, 0, PAGE_SIZE);
      a->desc = &block_descs[desc_idx];
      a->cnt = block_descs[desc_idx].blocks_per_arnea;
      a->large = false;
      uint32_t block_idx;
      enum intr_status old_status = intr_disable();
      for (block_idx = 0; block_idx < block_descs[desc_idx].blocks_per_arnea;
           block_idx++) {
        b = arena2block(a, block_idx);
        ASSERT(!element_find(&a->desc->free_list, &b->free_elem));
        list_append(&a->desc->free_list, &b->free_elem);
      }
      intr_set_status(old_status);
    }
    b = elem2entry(struct mem_block, free_elem,
                   list_pop(&(block_descs[desc_idx].free_list)));
    memset(b, 0, block_descs[desc_idx].block_size);
    a = block2arena(b);
    a->cnt--;
    lock_release(&mem_pool->lock);
    return (void*)b;
  }
}

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
    struct task_struct* cur = running_thread();
    bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_btmp, pg_cnt);
    if (bit_idx_start == -1) {
      return NULL;
    }
    while (cnt < pg_cnt) {
      bitmap_set(&cur->userprog_vaddr.vaddr_btmp, bit_idx_start + cnt, 1);
      cnt++;
    }
    vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PAGE_SIZE;

    // start process
    ASSERT((uint32_t)vaddr_start < (0xc0000000 - PAGE_SIZE));
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

  lock_init(&kernel_pool.lock);
  lock_init(&user_pool.lock);

  kernel_vaddr.vaddr_btmp.btmp_bytes_len = kbm_length;
  kernel_vaddr.vaddr_btmp.bits =
      (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
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

void pfree(uint32_t pd_phy_addr) {
  struct pool* m_pool;
  uint32_t bit_idx = 0;
  if (pd_phy_addr >= user_pool.phy_addr_start) {
    m_pool = &user_pool;
    bit_idx = (pd_phy_addr - user_pool.phy_addr_start) / PAGE_SIZE;
  } else {
    m_pool = &kernel_pool;
    bit_idx = (pd_phy_addr - kernel_pool.phy_addr_start) / PAGE_SIZE;
  }
  bitmap_set(&m_pool->pool_bitmap, bit_idx, 0);
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

static void page_table_pte_remove(uint32_t vaddr) {
  uint32_t* pte = pte_ptr(vaddr);
  *pte &= ~PG_P_1;
  asm volatile("invlpg %0" ::"m"(vaddr) : "memory");
}

static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
  uint32_t bit_idx = 0, vaddr = (uint32_t)_vaddr, cnt = 0;
  if (pf == PF_KERNEL) {
    bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PAGE_SIZE;
    while (cnt < pg_cnt) {
      bitmap_set(&kernel_vaddr.vaddr_btmp, bit_idx + cnt++, 0);
    }
  } else {
    struct task_struct* cur = running_thread();
    bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PAGE_SIZE;
    while (cnt < pg_cnt) {
      bitmap_set(&cur->userprog_vaddr.vaddr_btmp, bit_idx + cnt++, 0);
    }
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

void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
  uint32_t pg_phy_addr;
  uint32_t vaddr = (uint32_t)_vaddr, page_cnt = 0;
  ASSERT(pg_cnt >= 1 && vaddr % PAGE_SIZE == 0);
  pg_phy_addr = addr_v2p(vaddr);

  ASSERT(pg_phy_addr % PAGE_SIZE == 0 && pg_phy_addr >= 0x102000);

  if (pg_phy_addr >= user_pool.phy_addr_start) {
    vaddr -= PAGE_SIZE;
    while (page_cnt < pg_cnt) {
      vaddr += PAGE_SIZE;
      pg_phy_addr = addr_v2p(vaddr);
      ASSERT(pg_phy_addr % PAGE_SIZE == 0 &&
             pg_phy_addr >= user_pool.phy_addr_start);
      pfree(pg_phy_addr);
      page_table_pte_remove(vaddr);
      page_cnt++;
    }
    vaddr_remove(pf, _vaddr, pg_cnt);
  } else {
    vaddr -= PAGE_SIZE;
    while (page_cnt < pg_cnt) {
      vaddr += PAGE_SIZE;
      pg_phy_addr = addr_v2p(vaddr);
      ASSERT(pg_phy_addr % PAGE_SIZE == 0 &&
             pg_phy_addr >= kernel_pool.phy_addr_start);
      pfree(pg_phy_addr);
      page_table_pte_remove(vaddr);
      page_cnt++;
    }
    vaddr_remove(pf, _vaddr, pg_cnt);
  }
}

void sys_free(void* ptr) {
  ASSERT(ptr != NULL);
  enum pool_flags pf;
  struct pool* mem_pool;
  if (running_thread()->pgdir == NULL) {
    ASSERT((uint32_t)ptr >= K_HEAP_START);
    pf = PF_KERNEL;
    mem_pool = &kernel_pool;
  } else {
    pf = PF_USER;
    mem_pool = &user_pool;
  }
  lock_acquire(&mem_pool->lock);
  struct mem_block* b = ptr;
  struct arena* a = block2arena(b);
  ASSERT(a->large == 0 || a->large == 1);

  if (a->desc == NULL && a->large) {
    mfree_page(pf, a, a->cnt);
  } else {
    list_append(&a->desc->free_list, &b->free_elem);
    if (++a->cnt == a->desc->blocks_per_arnea) {
      for (uint32_t idx = 0; idx < a->desc->blocks_per_arnea; idx++) {
        struct mem_block* b = arena2block(a, idx);
        ASSERT(element_find(&a->desc->free_list, &b->free_elem));
        list_remove(&b->free_elem);
      }
      mfree_page(pf, a, 1);
    }
  }
  lock_release(&mem_pool->lock);
}

void* get_kernel_pages(uint32_t pg_cnt) {
  void* vaddr_start = malloc_page(PF_KERNEL, pg_cnt);
  if (vaddr_start == NULL) return NULL;
  memset(vaddr_start, 0, PAGE_SZIE * pg_cnt);
  return vaddr_start;
}

void* get_user_pages(uint32_t pg_cnt) {
  lock_acquire(&user_pool.lock);
  void* vaddr_start = malloc_page(PF_USER, pg_cnt);
  if (vaddr_start != NULL) {
    memset(vaddr_start, 0, PAGE_SIZE * pg_cnt);
  }
  lock_release(&user_pool.lock);
  return vaddr_start;
}

void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
  struct pool* m_pool = pf == PF_KERNEL ? &kernel_pool : &user_pool;
  lock_acquire(&m_pool->lock);

  struct task_struct* cur = running_thread();
  int32_t idx = -1;

  if (cur->pgdir != NULL && pf == PF_USER) {
    idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PAGE_SIZE;
    ASSERT(idx > 0)
    bitmap_set(&cur->userprog_vaddr.vaddr_btmp, idx, 1);
  } else if (cur->pgdir == NULL && pf == PF_KERNEL) {
    idx = (vaddr - kernel_vaddr.vaddr_start) / PAGE_SIZE;
    ASSERT(idx > 0)
    bitmap_set(&kernel_vaddr.vaddr_btmp, idx, 1);
  } else {
    PANIC(
        "get_a_page:not allow kernel alloc userspace or user alloc kernelspace "
        "by get_a_page");
  }

  void* page_phyaddr = palloc(m_pool);

  if (page_phyaddr == NULL) {
    return NULL;
  }

  page_table_add((void*)vaddr, page_phyaddr);
  lock_release(&m_pool->lock);
  return (void*)vaddr;
}

uint32_t addr_v2p(uint32_t vaddr) {
  uint32_t* ptr = pte_ptr(vaddr);

  return ((*ptr & 0xfffff000) + (vaddr & 0x00000fff));
}

void mem_init() {
  put_str("mem init start\n");
  uint32_t total_mem_bytes = *TOTAL_MEMORY_BYTES_ADDR;
  mem_pool_init(total_mem_bytes);
  block_desc_init(k_block_descs);
  put_str("mem init doen\n");
}