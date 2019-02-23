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
  for (int8_t i = 0; i < 8; i++) {
    buf[i] = num % 16 >= 10 ? num % 16 - 10 + 'A' : num % 16 - 0 + '0';
    num = num / 16;
    if (num == 0) {
      break;
    }
  }
  int8_t i = N - 1;
  while (i >= 0) {
    if (buf[i] != '0') {
      break;
    }
    i--;
  }
  while (i >= 0) {
    put_char(buf[i]);
    i--;
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