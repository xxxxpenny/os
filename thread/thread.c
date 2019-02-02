#include "thread.h"
#include "debug.h"
#include "global.h"
#include "interrupt.h"
#include "memory.h"
#include "print.h"
#include "stdint.h"
#include "string.h"

#define PAGE_SIZE 4096

struct task_struct* main_thread;
struct list thread_list_ready;
struct list thread_list_all;
static struct list_element* thread_tag;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

struct task_struct* running_thread() {
  uint32_t esp;
  asm volatile("mov %%esp, %0" : "=g"(esp));
  return (struct task_struct*)(esp & 0xfffff000);
}

static void kernel_thread(thread_func* function, void* args) {
  // 开中断,避免后面的时钟中断被屏蔽,无法调度
  intr_enable();
  function(args);
}

void create_therad(struct task_struct* pthread, thread_func function,
                   void* args) {
  pthread->self_kstack -= sizeof(struct intr_stack);

  // 留出线程栈空间,因为现在的赋值,是从低地址到高地址的
  // 不留出的话,会造成覆盖 intr_stack
  pthread->self_kstack -= sizeof(struct thread_stack);

  struct thread_stack* kthread_stack =
      (struct thread_stack*)pthread->self_kstack;
  kthread_stack->eip = kernel_thread;
  kthread_stack->function = function;
  kthread_stack->func_args = args;
  kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi =
      kthread_stack->esi = 0;
}

void init_thread(struct task_struct* pthread, char* name, uint8_t priority) {
  memset(pthread, 0, sizeof(pthread));
  strcpy(pthread->name, name);
  if (pthread == main_thread) {
    pthread->status = TASK_RUNNING;
  } else {
    pthread->status = TASK_READY;
  }
  pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PAGE_SIZE);
  pthread->priority = priority;
  pthread->ticks = priority;
  pthread->elapsed_ticks = 0;
  pthread->pgdir = NULL;
  pthread->stack_magic = 0x19960618;
}

struct task_struct* thread_start(char* name, uint8_t priority,
                                 thread_func function, void* args) {
  struct task_struct* pthread = get_kernel_pages(1);
  init_thread(pthread, name, priority);
  create_therad(pthread, function, args);
  ASSERT(!element_find(&thread_list_ready, &pthread->general_tag));
  list_append(&thread_list_ready, &pthread->general_tag);
  ASSERT(!element_find(&thread_list_all, &pthread->list_all_tag));
  list_append(&thread_list_all, &pthread->list_all_tag);
  /*
  asm volatile(
      "movl %0, %%esp;\
      pop %%ebp;\
      pop %%ebx;\
      pop %%edi;\
      pop %%esi;\
      ret" ::"g"(pthread->self_kstack)
      : "memory");
  **/
  return pthread;
}

static void make_main_thread() {
  // main thread早就存在了.
  // mov esp, 0xc009f000
  // pcb 地址为0xc009c000
  main_thread = running_thread();
  init_thread(main_thread, "main", 31);
  ASSERT(!element_find(&thread_list_all, &main_thread->list_all_tag));
  list_append(&thread_list_all, &main_thread->list_all_tag);
}

void schedule() {
  ASSERT(intr_get_status() == INTR_OFF);
  struct task_struct* cur_thread = running_thread();
  if (cur_thread->status == TASK_RUNNING) {
    ASSERT(!element_find(&thread_list_ready, &cur_thread->general_tag));
    list_append(&thread_list_ready, &cur_thread->general_tag);
    cur_thread->ticks = cur_thread->priority;
    cur_thread->status = TASK_READY;
  }

  ASSERT(!list_empty(&thread_list_ready));
  thread_tag = NULL;
  thread_tag = list_pop(&thread_list_ready);
  struct task_struct* next =
      elem2entry(struct task_struct, general_tag, thread_tag);
  next->status = TASK_RUNNING;
  switch_to(cur_thread, next);
}

void thread_init() {
  put_str("thread init start\n");
  list_init(&thread_list_all);
  list_init(&thread_list_ready);
  make_main_thread();
  put_str("thread init done\n");
}
