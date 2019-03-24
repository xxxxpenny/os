#ifndef __LIB_KERNEL_IO_H
#define __LIB_KERNEL_IO_H
#include "lib/stdint.h"

// 写一个字节
static inline void outb(uint16_t port, uint8_t data) {
  asm volatile("outb %b0,%w1" ::"a"(data), "Nd"(port));
}

// 写入多个字 2 byte
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
  asm volatile("cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(addr));
}

// 读入一个字节
static inline uint8_t inb(uint16_t port) {
  uint8_t data;
  asm volatile("inb %w1,%b0" : "=a"(data) : "Nd"(port));
  return data;
}

// 读入多个字
static inline void insw(uint16_t port, const void* addr, uint32_t word_cnt) {
  asm volatile("cld; rep insw"
               : "+D"(addr), "+c"(word_cnt)
               : "d"(port)
               : "memory");
}
#endif