#include "userprog/process.h"
#include "device/console.h"
#include "kernel/debug.h"
#include "kernel/global.h"
#include "kernel/interrupt.h"
#include "kernel/memory.h"
#include "lib/string.h"
#include "userprog/tss.h"

extern void intr_exit();

void start_process(void* filename_) {
  void* function = filename_;
  struct task_struct* cur = running_thread();
  cur->self_kstack += sizeof(struct thread_stack);
  struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;

  proc_stack->edi = 0;
  proc_stack->esi = 0;
  proc_stack->ebp = 0;
  proc_stack->esp_dummy = 0;

  proc_stack->ebx = 0;
  proc_stack->edx = 0;
  proc_stack->ecx = 0;
  proc_stack->eax = 0;

  proc_stack->gs = 0;
  proc_stack->ds = SELECTOR_U_DATA;
  proc_stack->es = SELECTOR_U_DATA;
  proc_stack->fs = SELECTOR_U_DATA;

  proc_stack->eip = function;
  proc_stack->cs = SELECTOR_U_CODE;
  proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
  // 对应 memory.c 的vaddr_get() ASSERT
  proc_stack->esp =
      (void*)((uint32_t)(get_a_page(PF_USER, USER_STACK3_VADDR) +
      PAGE_SIZE));
  proc_stack->ss = SELECTOR_U_DATA;
  asm volatile(
      "movl %0, %%esp; \
      jmp intr_exit" ::"g"(proc_stack)
      : "memory");
}


// 激活页表
void page_dir_activate(struct task_struct* thread) {
  /**
   *  注意,目前的实现,线程是线程,进程是进程
   *  进程有自己的地址空间, 线程是共享的内存的地址空间
   *  cr3 寄存器存放的是页目录地址,所以需要更新
   **/
  uint32_t pagedir_phy_addr = 0x100000;
  if (thread->pgdir != NULL) {
    // 进程
    pagedir_phy_addr = addr_v2p((uint32_t)thread->pgdir);
  }

  asm volatile("movl %0, %%cr3" ::"r"(pagedir_phy_addr) : "memory");
}

void process_activate(struct task_struct* thread) {
  ASSERT(thread != NULL);

  page_dir_activate(thread);

  if (thread->pgdir != NULL) {
    update_tss_esp(thread);
  }
}

uint32_t* create_page_dir() {
  // 进程的页表不能被进程直接访问
  // 故需要分配在内核空间
  uint32_t* page_dir_vaddr = get_kernel_pages(1);

  if (page_dir_vaddr == NULL) {
    console_put_str("create_page_dir: get_kernel_page failed");
    return NULL;
  }

  memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4),
         (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);

  uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);

  page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
  return page_dir_vaddr;
}

void create_user_vaddr_bitmap(struct task_struct* user_prog) {
  user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
  uint32_t bitmap_pg_cnt =
      DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
  user_prog->userprog_vaddr.vaddr_btmp.bits = get_kernel_pages(bitmap_pg_cnt);
  user_prog->userprog_vaddr.vaddr_btmp.btmp_bytes_len =
      (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
  bitmap_init(&user_prog->userprog_vaddr.vaddr_btmp);
}

void process_execute(void* filename, char* name) {
  struct task_struct* thread = get_kernel_pages(1);
  init_thread(thread, name, default_prio);
  create_user_vaddr_bitmap(thread);
  create_therad(thread, start_process, filename);
  thread->pgdir = create_page_dir();

  enum intr_status old_status = intr_disable();

  ASSERT(!element_find(&thread_list_ready, &thread->general_tag));
  list_append(&thread_list_ready, &thread->general_tag);
  ASSERT(!element_find(&thread_list_all, &thread->list_all_tag));
  list_append(&thread_list_all, &thread->list_all_tag);

  intr_set_status(old_status);
}
