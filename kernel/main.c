#include "init.h"
#include "interrupt.h"
#include "print.h"
#include "console.h"
#include "thread.h"

void k_thread_a(void* args);

void k_thread_b(void* args);

int main(void) {
  put_str("I am kernel!\n");
  init_all();
  thread_start("k_thread_a", 31, k_thread_a, "argA ");
  thread_start("k_thread_b", 8, k_thread_b, "argB ");
  intr_enable();
  while (1) {
    console_put_str("Main ");
  }
  return 0;
}

void k_thread_a(void* args) {
  char* str = args;
  while (1) {
    console_put_str(str);
  }
}

void k_thread_b(void* args) {
  char* str = args;
  while (1) {
    console_put_str(str);
  }
}
