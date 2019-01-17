#include "debug.h"
#include "init.h"
#include "print.h"

int main(void) {
  put_str("I am kernel!\n");
  init_all();
  ASSERT(1 == 2);
  while (1)
    ;
}
