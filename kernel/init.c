#include "init.h"
#include "interrupt.h"
#include "memory.h"
#include "lib/kernel/print.h"
#include "thread/thread.h"
#include "device/timer.h"
#include "device/console.h"
#include "device/keyboard.h"
#include "userprog/tss.h"
#include "userprog/syscall-init.h"

void init_all() {
  put_str("init all\n");
  idt_init();
  mem_init();
  thread_init();
  timer_init();
  console_init();
  keyboard_init();
  tss_init();
  syscall_init();
}