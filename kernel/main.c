#include "init.h"
#include "print.h"
#include "thread.h"

void k_thread_a(void* args);

int main(void) {
  put_str("I am kernel!\n");
  init_all();
  thread_start("k_thread_a", 31, k_thread_a, "argA");
  return 0;
}

void k_thread_a(void* args) {
  char* str = args;
  while (1) {
    put_str(str);
  }
}
