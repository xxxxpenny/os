#include "debug.h"
#include "interrupt.h"
#include "print.h"

void panic_spin(char* filename, int line, const char* func,
                const char* condition) {
  intr_disable();
  put_str("filename: ");
  put_str(filename);
  put_char('\n');
  put_str("line: 0x");
  put_int(line);
  put_char('\n');
  put_str("func: ");
  put_str((char*)func);
  put_char('\n');
  put_str("condition: ");
  put_str((char*)condition);
  put_char('\n');
  while (1)
    ;
}