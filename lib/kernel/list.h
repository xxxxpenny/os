#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

#include "kernel/global.h"

#define offset(struct_type, member) (int32_t)(&(((struct_type*)0)->member))
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
  (struct_type*)((int32_t)elem_ptr - offset(struct_type, struct_member_name))

struct list_element {
  struct list_element* prev;
  struct list_element* next;
};

// 不用指针,因为需要初始化指针,开辟空间
// 还没有实现 malloc
struct list {
  struct list_element head;
  struct list_element tail;
};

typedef bool(function)(struct list_element*, int32_t arg);

void list_init(struct list* list);
void list_insert_before(struct list_element* before,
                        struct list_element* element);
void list_push(struct list* list, struct list_element* element);
struct list_element* list_pop(struct list* list);
void list_iterate(struct list* list);
void list_append(struct list* list, struct list_element* element);
void list_remove(struct list_element* element);
bool list_empty(struct list* list);
uint32_t list_len(struct list* list);
struct list_element* list_traversal(struct list* list, function func,
                                    int32_t arg);
bool element_find(struct list* list, struct list_element* element);

#endif