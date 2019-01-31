#include "timer.h"
#include "debug.h"
#include "interrupt.h"
#include "io.h"
#include "print.h"
#include "thread.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT 0x40
#define COUNTER0_NO 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_COUNTER_PORT 0x43

uint32_t ticks;

static void frequency_set(uint8_t port, uint8_t no, uint8_t rwl, uint8_t mode,
                          uint16_t value) {
  outb(PIT_COUNTER_PORT, no << 6 | rwl << 4 | mode << 1);
  outb(port, (uint8_t)value);
  outb(port, (uint8_t)value >> 8);
}

static void intr_timer_handler() {
  struct task_struct* cur_thread = running_thread();
  ASSERT(cur_thread->stack_magic == 0x19960618);
  cur_thread->elapsed_ticks++;
  ticks++;
  if (cur_thread->ticks == 0) {
    schedule();
  } else {
    cur_thread->ticks--;
  }
}

void timer_init() {
  put_str("   timer init start\n");
  frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE,
                COUNTER_VALUE);
  register_intr_handler(0x20, intr_timer_handler);
  put_str("   timer init done\n");
}