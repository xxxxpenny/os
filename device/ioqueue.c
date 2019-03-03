#include "ioqueue.h"
#include "debug.h"
#include "interrupt.h"

void ioqueue_init(struct ioqueue* queue) {
  lock_init(&queue->lock);
  queue->producer = queue->consumer = NULL;
  queue->head = queue->tail = 0;
}

static int32_t next_pos(int32_t pos) { return (pos + 1) % bufsize; }

bool ioq_full(struct ioqueue* queue) {
  ASSERT(intr_get_status() == INTR_OFF);
  return next_pos(queue->head) == queue->tail;
}

static bool ioq_empty(struct ioqueue* queue) {
  ASSERT(intr_get_status() == INTR_OFF);
  return queue->head == queue->tail;
}

static void ioq_wait(struct task_struct** waiter) {
  ASSERT(*waiter == NULL && waiter != NULL);
  *waiter = running_thread();
  thread_block(TASK_BLOCKED);
}

static void wakeup(struct task_struct** waiter) {
  ASSERT(*waiter != NULL);
  thread_unblock(*waiter);
  *waiter = NULL;
}

char ioq_getchar(struct ioqueue* queue) {
  ASSERT(intr_get_status() == INTR_OFF);
  while (ioq_empty(queue)) {
    lock_acquire(&queue->lock);
    ioq_wait(&queue->consumer);
    lock_release(&queue->lock);
  }
  char byte = queue->buf[queue->tail];
  queue->tail = next_pos(queue->tail);

  if (queue->producer != NULL) {
    wakeup(&queue->producer);
  }

  return byte;
}

void ioq_putchar(struct ioqueue* queue, char byte) {
  ASSERT(intr_get_status() == INTR_OFF);
  while (ioq_full(queue)) {
    lock_acquire(&queue->lock);
    ioq_wait(&queue->producer);
    lock_release(&queue->lock);
  }
  queue->buf[queue->head] = byte;
  queue->head = next_pos(queue->head);

  if (queue->consumer != NULL) {
    wakeup(&queue->consumer);
  }
}