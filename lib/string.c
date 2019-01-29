#include "string.h"
#include "debug.h"
#include "global.h"

void memset(void* dst, uint8_t value, uint32_t size) {
  ASSERT(dst != NULL);
  uint8_t* _dst = (uint8_t*)dst;
  while (size-- > 0) {
    *_dst++ = value;
  }
}

void memcpy(void* dst, const void* src, uint32_t size) {
  ASSERT(src != NULL && dst != NULL);
  uint8_t* _dst = (uint8_t*)dst;
  const uint8_t* _src = (uint8_t*)src;
  while (size-- > 0) {
    *_dst++ = *_src++;
  }
}

// a == b  0
// a < b   -1
// a > b   1
int8_t memcmp(const void* a, const void* b, uint32_t size) {
  ASSERT(a != NULL || b != NULL);
  const uint8_t* _a = (uint8_t*)a;
  const uint8_t* _b = (uint8_t*)b;
  while (size-- > 0) {
    if (*_a != *_b) {
      return *_a > *_b ? 1 : -1;
    }
    _a++;
    _b++;
  }
  return 0;
}

char* strcpy(char* dst, const char* src) {
  ASSERT(dst != NULL && src != NULL);
  char* ret = dst;
  while ((*dst++ = *src++))
    ;
  return ret;
}

uint32_t strlen(const char* str) {
  ASSERT(str != NULL);
  const char* p = str;
  while (*p++)
    ;
  return p - str - 1;
}

int8_t strcmp(const char* a, const char* b) {
  ASSERT(a != NULL && b != NULL);
  uint8_t ret = 0;
  while (!(ret = *a - *b) && *a) {
    a++;
    b++;
  }
  if (ret < 0) {
    return -1;
  } else if (ret > 0) {
    return 1;
  }
  return 0;
}

// 第一次出现
char* strchr(const char* str, const uint8_t ch) {
  ASSERT(str != NULL);
  while (*str != '\0') {
    if (*str == ch) {
      return (char*)str;
    }
    str++;
  }

  return NULL;
}

// 最后一次出现
char* strrchr(const char* str, const uint8_t ch) {
  ASSERT(str != NULL);
  char* last_ch = NULL;
  while (*str != '\0') {
    if (*str == ch) {
      last_ch = (char*)str;
    }
    str++;
  }
  return last_ch;
}

char* strcat(char* dst, const char* src) {
  ASSERT(dst != NULL && src != NULL);
  char* ret = dst;
  while (*dst++)
    ;
  dst--;
  while ((*dst++ = *src++))
    ;
  return ret;
}

uint32_t strchrs(const char* str, uint8_t ch) {
  uint32_t count = 0;
  while (*str++ != '\0') {
    if (*str == ch) {
      count++;
    }
  }
  return count;
}