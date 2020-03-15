#include "device/console.h"
#include "init.h"
#include "interrupt.h"
#include "kernel/memory.h"
#include "lib/kernel/print.h"
#include "lib/stdio.h"
#include "lib/user/syscall.h"
#include "thread/thread.h"
#include "userprog/process.h"
#include "userprog/syscall-init.h"

void k_thread_a(void* args);
void k_thread_b(void* args);
void userprog_a();
void userprog_b();

int main(void) {
  put_str("I am kernel!\n");
  init_all();
  intr_enable();
  process_execute(userprog_a, "userprog_a");
  process_execute(userprog_b, "userprog_b");
  thread_start("k_thread_a", 31, k_thread_a, "argA ");
  thread_start("k_thread_b", 31, k_thread_b, "argB ");

  while (1)
    ;

  return 0;
}

void k_thread_a(void* args) {
  void* addr1 = sys_malloc(256);
  void* addr2 = sys_malloc(255);
  void* addr3 = sys_malloc(254);
  console_put_str("thread_a malloc addr:0x");
  console_put_int((uint32_t)addr1);
  console_put_char(',');
  console_put_int((uint32_t)addr2);
  console_put_char(',');
  console_put_int((uint32_t)addr3);
  console_put_char('\n');
  uint32_t cpu_delay = 10000;
  while (--cpu_delay > 0)
    ;
  sys_free(addr1);
  sys_free(addr2);
  sys_free(addr3);
  while (1)
    ;
}

void k_thread_b(void* args) {
  void* addr1 = sys_malloc(256);
  void* addr2 = sys_malloc(255);
  void* addr3 = sys_malloc(254);
  console_put_str("thread_b malloc addr:0x");
  console_put_int((uint32_t)addr1);
  console_put_char(',');
  console_put_int((uint32_t)addr2);
  console_put_char(',');
  console_put_int((uint32_t)addr3);
  console_put_char('\n');
  uint32_t cpu_delay = 10000;
  while (--cpu_delay > 0)
    ;
  sys_free(addr1);
  sys_free(addr2);
  sys_free(addr3);
  while (1)
    ;
}

void userprog_a() {
  void* addr1 = malloc(256);
  void* addr2 = malloc(255);
  void* addr3 = malloc(254);
  printf("prog_a molloc addr,0x%x,0x%x,0x%x\n", (uint32_t)addr1,
         (uint32_t)addr2, (uint32_t)addr3);
  uint32_t cpu_delay = 10000;
  while (--cpu_delay > 0)
    ;
  free(addr1);
  free(addr2);
  free(addr3);
  while(1);
}

void userprog_b() {
  void* addr1 = malloc(256);
  void* addr2 = malloc(255);
  void* addr3 = malloc(254);
  printf("prog_b molloc addr,0x%x,0x%x,0x%x\n", (uint32_t)addr1,
         (uint32_t)addr2, (uint32_t)addr3);
  uint32_t cpu_delay = 10000;
  while (--cpu_delay > 0)
    ;
  free(addr1);
  free(addr2);
  free(addr3);
  while(1);
}
