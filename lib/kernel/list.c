#include "list.h"
#include "kernel/interrupt.h"

void list_init(struct list* list) {
  list->head.prev = NULL;
  list->head.next = &list->tail;
  list->tail.prev = &list->head;
  list->tail.next = NULL;
}

void list_insert_before(struct list_element* before,
                        struct list_element* element) {
  enum intr_status old_status = intr_disable();
  before->prev->next = element;
  element->prev = before->prev;
  element->next = before;
  before->prev = element;
  intr_set_status(old_status);
}

void list_push(struct list* list, struct list_element* element) {
  list_insert_before(list->head.next, element);
}

struct list_element* list_pop(struct list* list) {
  struct list_element* element = list->head.next;
  list_remove(element);
  return element;
}

void list_iterate(struct list* list) {}

void list_append(struct list* list, struct list_element* element) {
  list_insert_before(&list->tail, element);
}

void list_remove(struct list_element* element) {
  enum intr_status old_status = intr_disable();
  element->prev->next = element->next;
  element->next->prev = element->prev;
  intr_set_status(old_status);
}

bool list_empty(struct list* list) {
  return list->head.next == &list->tail ? true : false;
}

uint32_t list_len(struct list* list) {
  uint32_t len = 0;
  struct list_element* p = list->head.next;
  while (p != &list->tail) {
    len++;
    p = p->next;
  }
  return len;
}

struct list_element* list_traversal(struct list* list, function func,
                                    int32_t arg) {
  if (list_empty(list)) return NULL;
  struct list_element* p = list->head.next;
  while (p != &list->tail) {
    if (func(p, arg)) return p;
    p = p->next;
  }
  return NULL;
}

bool element_find(struct list* list, struct list_element* element) {
  struct list_element* p = list->head.next;
  while (p != &list->tail) {
    if (p == element) return true;
    p = p->next;
  }
  return false;
}