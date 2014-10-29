/**
 * @file
 *
 * @date 17.12.10
 * @author Nikolay Korotky
 */

#ifndef X86_TRAPS_H_
#define X86_TRAPS_H_

#ifndef __ASSEMBLER__

#include <stdint.h>
/*
 * There are 256 IDT entries (each entry is 8 bytes)
 * Vectors 0...31 : system traps and exceptions
 * Vectors 32...237 : device interrupts
 * Vectors 238...255 : special interrupts
 */

/*
 * Hardware-generated exceptions
 */
#define X86_T_DIVIDE_ERROR          0x00 /* Division by zero */
#define X86_T_DEBUG                 0x01 /* Debugger */
#define X86_T_NMI                   0x02 /* Non-maskable interrupt */
#define X86_T_INT3                  0x03 /* Breakpoint */
#define X86_T_OVERFLOW              0x04 /* overflow test */
#define X86_T_OUT_OF_BOUNDS         0x05 /* bounds check */
#define X86_T_INVALID_OPCODE        0x06 /* invalid op code */
#define X86_T_NO_FPU                0x07 /* Coprocessor not available */
#define X86_T_DOUBLE_FAULT          0x08 /* double fault */
#define X86_T_FPU_FAULT             0x09 /* Coprocessor Segment Overrun */
/* BIOS interrupt call for video services 0x10 */
#define X86_T_SEGMENT_NOT_PRESENT   0x0A /* Invalid Task State Segment (TSS) */
#define X86_T_STACK_FAULT           0x0C /* Stack Fault */
#define X86_T_GENERAL_PROTECTION    0x0D /* General Protection */
#define X86_T_PAGE_FAULT            0x0E /* Page Fault */

typedef struct pt_regs {
	/* Pushed by SAVE_ALL. */
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t rax;
	uint64_t gs;
	uint64_t fs;
	uint64_t es;
	uint64_t ds;

	/* Pushed at the very beginning of entry. */
	uint64_t trapno;

	/* In some cases pushed by processor, in some - by us. */
	uint64_t err;

	/* Pushed by processor. */
	uint64_t eip;
	uint64_t cs;
	uint64_t eflags;

	/* Pushed by processor, if switching of rings occurs. */
	uint64_t rsp;
	uint64_t ss;
} pt_regs_t;

extern void idt_set_gate(uint8_t nr, uint64_t base, uint16_t sel, uint8_t attr);
extern void idt_init(void);
extern void gdt_init(void);

#else

/* Use for exception which doesn't push error code. */
#define EXCEPTION(n, name)  \
	.globl name            ;\
name:                      ;\
	pushq	$(0)           ;\
	pushq	$(n)           ;\
	jmp	excep_stub

/* Use for exception which pushes error code. */
#define EXCEPTION_ERR(n, name)  \
	.globl name                ;\
name:                          ;\
	pushq	$(n)               ;\
	jmp	excep_stub

#define IRQ_ENTRY(n)      \
	.globl irq##n        ;\
irq##n:                  ;\
	pushq   $(0)         ;\
	pushq   $(32 + n)    ;\
	jmp     irq_stub

#endif /* __ASSEMBLER__ */

#endif /* X86_TRAPS_H_ */
