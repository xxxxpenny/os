#include "debug.h"
#include "init.h"
#include "memory.h"
#include "print.h"

int main(void) {
  put_str("I am kernel!\n");
  init_all();
  void *addr = get_kernel_pages(3);
  put_str("\n virtual addr is: ");
  put_int((int)addr);
  put_str("\n");
  while (1)
    ;
  return 0;
}
