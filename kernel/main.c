#include "device/console.h"
#include "init.h"
#include "interrupt.h"
#include "lib/kernel/print.h"
#include "thread/thread.h"
#include "userprog/process.h"

void k_thread_a(void* args);
void k_thread_b(void* args);
void userprog_a();
void userprog_b();

int test_var_a = 1, test_var_b = 1;

int main(void) {
  put_str("I am kernel!\n");
  init_all();

  thread_start("k_thread_a", 31, k_thread_a, "argA ");
  thread_start("k_thread_b", 8, k_thread_b, "argB ");
  process_execute(userprog_a, "a");
  process_execute(userprog_b, "b");

  intr_enable();
  while (1)
    ;

  return 0;
}

void k_thread_a(void* args) {
  char* str = args;
  while (1) {
    console_put_str(" v_a:0x");
    console_put_int(test_var_a);
  }
}

void k_thread_b(void* args) {
  char* str = args;
  while (1) {
    console_put_str(" v_b:0x");
    console_put_int(test_var_b);
  }
}

void userprog_a() {
  while (true) {
    test_var_a++;
  }
}

void userprog_b() {
  while (true) {
    test_var_b++;
  }
}


