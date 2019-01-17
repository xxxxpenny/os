#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "print.h"
#include "stdint.h"

#define IDT_DESC_CNT 33

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21

#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl;popl %0" : "=g"(EFLAG_VAR))

// 中断门描述符
struct gate_desc {
  uint16_t func_offset_low_word;
  uint16_t selector;
  uint8_t dcount;
  uint8_t attribute;
  uint16_t func_offset_high_word;
};

static void general_intr_handler(uint8_t vec_nr);
static void exception_init();
static void make_idt_desc(struct gate_desc *p_desc, uint8_t attr,
                          intr_handler function);
static void idt_desc_init();
static void pic_init();

static struct gate_desc idt[IDT_DESC_CNT];

intr_handler idt_table[IDT_DESC_CNT];

char *intr_name[IDT_DESC_CNT];

extern intr_handler intr_entry_table[IDT_DESC_CNT];

static void general_intr_handler(uint8_t vec_nr) {
  if (vec_nr == 0x27 || vec_nr == 0x2f) {
    return;
  }
  put_str("int vector: 0x");
  put_int(vec_nr);
  put_char('\n');
}

static void exception_init() {
  for (int i = 0; i < IDT_DESC_CNT; i++) {
    idt_table[i] = general_intr_handler;
    intr_name[i] = "unknown";
  }
  intr_name[0] = "#DE Divide Error";
  intr_name[1] = "#DB Debug Exception";
  intr_name[2] = "NMI Interrupt";
  intr_name[3] = "#BP Breakpoint Exception";
  intr_name[4] = "#OF Overflow Exception";
  intr_name[5] = "#BR BOUND Range Exceeded Exception";
  intr_name[6] = "#UD Invalid Opcode Exception";
  intr_name[7] = "#NM Device Not Available Exception";
  intr_name[8] = "#DF Double Fault Exception";
  intr_name[9] = "Coprocessor Segment Overrun";
  intr_name[10] = "#TS Invalid TSS Exception";
  intr_name[11] = "#NP Segment Not Present";
  intr_name[12] = "#SS Stack Fault Exception";
  intr_name[13] = "#GP General Protection Exception";
  intr_name[14] = "#PF Page-Fault Exception";
  // intr_name[15] 第15项是intel保留项，未使用
  intr_name[16] = "#MF x87 FPU Floating-Point Error";
  intr_name[17] = "#AC Alignment Check Exception";
  intr_name[18] = "#MC Machine-Check Exception";
  intr_name[19] = "#XF SIMD Floating-Point Exception";
}

static void make_idt_desc(struct gate_desc *p_desc, uint8_t attr,
                          intr_handler function) {
  p_desc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
  p_desc->selector = SELECTOR_K_CODE;
  p_desc->dcount = 0;
  p_desc->attribute = attr;
  p_desc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init() {
  for (int i = 0; i < IDT_DESC_CNT; i++) {
    make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
  }
  put_str(" idt_desc_init done\n");
}

static void pic_init() {
  // 初始化主片
  outb(PIC_M_CTRL, 0x11);  // icw1
  outb(PIC_M_DATA, 0x20);  // icw2
  outb(PIC_M_DATA, 0x04);  // icw3
  outb(PIC_M_DATA, 0x01);  // icw4

  // 初始化从片
  outb(PIC_S_CTRL, 0x11);  // icw1
  outb(PIC_S_DATA, 0x28);  // icw2
  outb(PIC_S_DATA, 0x02);  // icw3
  outb(PIC_S_DATA, 0x01);  // icw4

  outb(PIC_M_DATA, 0xfe);  // 主片打开1号中断
  outb(PIC_S_DATA, 0xff);  // 从片全屏蔽

  put_str(" pic init end\n");
}

void idt_init() {
  put_str("idt init start\n");
  idt_desc_init();
  exception_init();
  pic_init();
  uint64_t idtr_op = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
  asm volatile("lidt %0" : : "m"(idtr_op));
}

enum intr_status intr_enable() {
  enum intr_status old_status = intr_get_status();
  if (old_status != INTR_ON) {
    asm volatile("sti");
  }
  return old_status;
}
enum intr_status intr_disable() {
  enum intr_status old_status = intr_get_status();
  if (old_status != INTR_OFF) {
    asm volatile("cli" ::: "memory");
  }
  return old_status;
}
enum intr_status intr_set_status(enum intr_status status) {
  return status == INTR_ON ? intr_enable() : intr_disable();
}
enum intr_status intr_get_status() {
  uint32_t eflag_var = 0;
  GET_EFLAGS(eflag_var);
  return (eflag_var & EFLAGS_IF) == EFLAGS_IF ? INTR_ON : INTR_OFF;
}