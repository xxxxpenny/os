#include "init.h"
#include "interrupt.h"
#include "memory.h"
#include "print.h"
#include "thread.h"
#include "timer.h"
#include "console.h"

void init_all() {
  put_str("init all\n");
  idt_init();
  mem_init();
  thread_init();
  timer_init();
  console_init();
}