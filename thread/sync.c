#include "thread/sync.h"
#include "kernel/debug.h"
#include "kernel/interrupt.h"
#include "lib/kernel/list.h"
#include "thread/thread.h"

void sema_init(struct semaphore* sema, uint8_t value) {
  sema->value = value;
  list_init(&sema->waiters);
}

void lock_init(struct lock* lock) {
  lock->holder = NULL;
  lock->holer_repeat_nr = 0;
  sema_init(&lock->semaphore, 1);
}

void sema_down(struct semaphore* sema) {
  enum intr_status old_status = intr_disable();
  while (sema->value == 0) {
    ASSERT(!element_find(&sema->waiters, &running_thread()->general_tag));
    if (element_find(&sema->waiters, &running_thread()->general_tag)) {
      PANIC("sema down: thread blocked has benn in waiter list");
    }
    list_append(&sema->waiters, &running_thread()->general_tag);
    block_thread(TASK_BLOCKED);
  }
  sema->value--;
  ASSERT(sema->value == 0);
  intr_set_status(old_status);
}

void sema_up(struct semaphore* sema) {
  enum intr_status old_status = intr_disable();
  ASSERT(sema->value == 0);
  if (!list_empty(&sema->waiters)) {
    struct task_struct* blocked_thread =
        elem2entry(struct task_struct, general_tag, list_pop(&sema->waiters));
    unblock_thread(blocked_thread);
  }
  sema->value++;
  ASSERT(sema->value == 1);
  intr_set_status(old_status);
}

void lock_acquire(struct lock* lock) {
  if (lock->holder != running_thread()) {
    sema_down(&lock->semaphore);
    lock->holder = running_thread();
    ASSERT(lock->holer_repeat_nr == 0);
    lock->holer_repeat_nr = 1;
  } else {
    lock->holer_repeat_nr++;
  }
}

void lock_release(struct lock* lock) {
  ASSERT(lock->holder == running_thread());
  if (lock->holer_repeat_nr > 1) {
    lock->holer_repeat_nr--;
    return;
  }
  ASSERT(lock->holer_repeat_nr == 1);
  lock->holder = NULL;
  lock->holer_repeat_nr = 0;
  sema_up(&lock->semaphore);
}
