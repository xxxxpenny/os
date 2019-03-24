#include "lib/kernel/print.h"
#include "lib/kernel/io.h"
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
  int8_t count = N - 1;
  while (count >= 0) {
    buf[count] = digits[num & 0xf];
    num = num >> 4;
    count--;
  }

  int8_t i = 0;
  for (i = 0; i < N; i++) {
    if (buf[i] != '0') {
      break;
    }
  }
  for (; i < N; i++) {
    put_char(buf[i]);
  }
}

void put_long(uint64_t num) {
  char buf[2 * N];
  char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  int8_t count = 2 * N - 1;
  while (count >= 0) {
    buf[count] = digits[num & 0xf];
    num = num >> 4;
    count--;
  }

  int8_t i = 0;
  for (; i < 2 * N; i++) {
    put_char(buf[i]);
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