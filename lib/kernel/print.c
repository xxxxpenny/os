#include "print.h"
#include "io.h"
#define N 8

void put_str(char* str) {
  while (*str != '\0') {
    put_char(*str);
    str++;
  }
}

void put_int(uint32_t num) {
  char buf[N];
  char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  int8_t n = N;
  while (--n >= 0) {
    buf[n] = digits[num & 15];
    num >>= 4;
  }
  int8_t i = 0;
  while (i < N) {
    if (buf[i] != '0') {
      break;
    }
    i++;
  }
  while (i < N) {
    put_char(buf[i]);
    i++;
  }
}

void set_cursor(uint32_t cursor) {
  cursor = (uint16_t)cursor;
  uint8_t high = (uint8_t)((cursor & 0xff00) >> 8);
  uint8_t low = (uint8_t)(cursor & 0x00ff);
  outb(0x03d4, 0x0e);
  outb(0x03d5, high);
  outb(0x03d4, 0x0f);
  outb(0x03d5, low);
}