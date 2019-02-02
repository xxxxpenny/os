#include "init.h"
#include "interrupt.h"
#include "print.h"
#include "thread.h"

void k_thread_a(void* args);

void k_thread_b(void* args);

int main(void) {
  put_str("I am kernel!\n");
  init_all();
  thread_start("k_thread_a", 31, k_thread_a, "11111");
  thread_start("k_thread_b", 8, k_thread_b, "22222");
  intr_enable();
  while (1) {
    intr_disable();
    put_str("33333");
    intr_enable();
  }

  return 0;
}

void k_thread_a(void* args) {
  char* str = args;
  while (1) {
    intr_disable();
    put_str(str);
    intr_enable();
  }
}

void k_thread_b(void* args) {
  char* str = args;
  while (1) {
    intr_disable();
    put_str(str);
    intr_enable();
  }
}
// #include "init.h"
// #include "interrupt.h"
// #include "print.h"
// #include "thread.h"

// void k_thread_a(void*);
// void k_thread_b(void*);
// int main(void) {
//   put_str("I am kernel\n");
//   init_all();

//   thread_start("k_thread_a", 31, k_thread_a, "argA ");
//   thread_start("k_thread_b", 8, k_thread_b, "argB ");

//   intr_enable();  // 打开中断,使时钟中断起作用
//   while (1) {
//     intr_disable();
//     put_str("333 ");
//     intr_enable();
//   };
//   return 0;
// }

// /* 在线程中运行的函数 */
// void k_thread_a(void* arg) {
//   /* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用
//    */
//   char* para = arg;
//   while (1) {
//     intr_disable();
//     put_str(para);
//     intr_enable();
//   }
// }

// /* 在线程中运行的函数 */
// void k_thread_b(void* arg) {
//   /* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用
//    */
//   char* para = arg;
//   while (1) {
//     intr_disable();
//     put_str(para);
//     intr_enable();
//   }
// }
