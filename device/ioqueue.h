#ifndef __DEVICDE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "sync.h"
#include "thread.h"

#define bufsize 64

struct ioqueue {
  struct lock lock;
  struct task_struct* producer;
  struct task_struct* consumer;
  char buf[bufsize];
  int32_t head;
  int32_t tail;
};

void ioqueue_init(struct ioqueue* queue);
bool ioq_full(struct ioqueue* queue);
char ioq_getchar(struct ioqueue* queue);
void ioq_putchar(struct ioqueue* queue, char byte);

#endif