#include "device/console.h"
#include "init.h"
#include "interrupt.h"
#include "lib/kernel/print.h"
#include "lib/stdio.h"
#include "lib/user/syscall.h"
#include "thread/thread.h"
#include "userprog/process.h"
#include "userprog/syscall-init.h"

void k_thread_a(void* args);
void k_thread_b(void* args);
void userprog_a();
void userprog_b();

int main(void) {
  put_str("I am kernel!\n");
  init_all();

  process_execute(userprog_a, "a");
  process_execute(userprog_b, "b");

  intr_enable();
  console_put_str(" main pid:0x");
  console_put_int(sys_getpid());
  console_put_char('\n');
  thread_start("k_thread_a", 31, k_thread_a, "argA ");
  thread_start("k_thread_b", 8, k_thread_b, "argB ");

  while (1)
    ;

  return 0;
}

void k_thread_a(void* args) {
  char* str = args;
  console_put_str(" \nthread_a_pid:0x");
  console_put_int(sys_getpid());
  console_put_char('\n');
  while (1)
    ;
}

void k_thread_b(void* args) {
  char* str = args;
  console_put_str(" \nthread_b_pid:0x");
  console_put_int(sys_getpid());
  console_put_char('\n');
  while (1)
    ;
}

void userprog_a() {
  char* name = "prog_a_pid";
  printf("\nI am %s, my pid:%d%c", name, getpid(), '\n');
  while (1)
    ;
}

void userprog_b() {
  char* name = "prog_b_pid";
  printf("\nI am %s, my pid:%d%c", name, getpid(), '\n');
  while (1)
    ;
}
