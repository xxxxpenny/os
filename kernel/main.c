#include "device/console.h"
#include "init.h"
#include "interrupt.h"
#include "lib/kernel/print.h"
#include "lib/user/syscall.h"
#include "thread/thread.h"
#include "userprog/process.h"
#include "userprog/syscall-init.h"

void k_thread_a(void* args);
void k_thread_b(void* args);
void userprog_a();
void userprog_b();

int prog_a_pid = 0, prog_b_pid = 0;

int main(void) {
  put_str("I am kernel!\n");
  init_all();

  process_execute(userprog_a, "a");
  process_execute(userprog_b, "b");

  intr_enable();
  console_put_str(" main pid:0x");
  console_put_int(getpid());
  console_put_char('\n');
  thread_start("k_thread_a", 31, k_thread_a, "argA ");
  thread_start("k_thread_b", 8, k_thread_b, "argB ");

  while (1)
    ;

  return 0;
}

void k_thread_a(void* args) {
  char* str = args;
  console_put_str(" thread_a_pid:0x");
  console_put_int(sys_getpid());
  console_put_char('\n');
  console_put_str(" prog_a_pid:0x");
  console_put_int(prog_a_pid);
  console_put_char('\n');
  while (1)
    ;
}

void k_thread_b(void* args) {
  char* str = args;
  console_put_str(" thread_b_pid:0x");
  console_put_int(sys_getpid());
  console_put_char('\n');
  console_put_str(" prog_b_pid:0x");
  console_put_int(prog_b_pid);
  console_put_char('\n');
  while (1)
    ;
}

void userprog_a() {
  prog_a_pid = getpid();
  while (1)
    ;
}

void userprog_b() {
  prog_b_pid = getpid();
  while (1)
    ;
}
