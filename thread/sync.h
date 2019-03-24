#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "lib/kernel/list.h"
#include "lib/stdint.h"
#include "thread/thread.h"

struct semaphore {
  uint8_t value;
  struct list waiters;
};

struct lock {
  struct task_struct* holder;
  struct semaphore semaphore;
  uint32_t holer_repeat_nr;
};

void lock_init(struct lock* lock);
void lock_acquire(struct lock* lock);
void lock_release(struct lock* lock);
#endif