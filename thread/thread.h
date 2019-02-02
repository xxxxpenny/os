#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "list.h"
#include "stdint.h"

typedef void thread_func(void*);

enum task_status {
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
  TASK_WAITING,
  TASK_HANGING,
  TASK_DIED
};

struct intr_stack {
  uint32_t vec_no;
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp_dummy;

  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  uint32_t gs;
  uint32_t fs;
  uint32_t es;
  uint32_t ds;

  uint32_t err_code;
  void (*eip)(void);
  uint32_t cs;
  uint32_t eflags;
  void* esp;
  uint32_t ss;
};

struct thread_stack {
  uint32_t ebp;
  uint32_t ebx;
  uint32_t edi;
  uint32_t esi;

  void (*eip)(thread_func* func, void* func_arg);

  void(*unused_retaddr);
  thread_func* function;
  void* func_args;
};

struct task_struct {
  uint32_t* self_kstack;
  enum task_status status;
  char name[16];
  uint8_t priority;

  // 每次调度的运行时间
  uint8_t ticks;

  // 任务运行时间
  uint32_t elapsed_ticks;

  // 线程在一般队列中的节点
  struct list_element general_tag;

  // 线程在线程队列中的节点 thread_list_all
  struct list_element list_all_tag;

  uint32_t* pgdir;

  uint32_t stack_magic;
};

struct task_struct* thread_start(char* name, uint8_t priority,
                                 thread_func function, void* args);
struct task_struct* running_thread();
void schedule();
void thread_init();

#endif