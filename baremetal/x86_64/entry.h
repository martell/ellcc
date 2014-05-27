/**
 * @file
 * @brief Definitions used by low-level trap handlers
 *
 * @date 21.09.11
 * @author Anton Bondarev
 */

#ifndef X86_ENTRY_H_
#define X86_ENTRY_H_

#include "gdt.h"

#ifdef __ASSEMBLER__

#define SAVE_ALL         \
	pushq   %rax;    \
	pushq   %rbp;    \
	pushq   %rdi;    \
	pushq   %rsi;    \
	pushq   %rdx;    \
	pushq   %rcx;    \
	pushq   %rbx;    \
	pushq   %r8;     \
	pushq   %r9;     \
	pushq   %r10;    \
	pushq   %r11;    \
	pushq   %r12;    \
	pushq   %r13;    \
	pushq   %r14;    \
	pushq   %r15;

#define RESTORE_ALL      \
	pop   %r15;      \
	pop   %r14;      \
	pop   %r13;      \
	pop   %r12;      \
	pop   %r11;      \
	pop   %r10;      \
	pop   %r9;       \
	pop   %r8;       \
	pop   %rbx;      \
	pop   %rcx;      \
	pop   %rdx;      \
	pop   %rsi;      \
	pop   %rdi;      \
	pop   %rbp;      \
	pop   %rax;      \
	add   $16, %rsp; \
	iret;

#define RESTORE_ALL_64   \
	pop   %r15;      \
	pop   %r14;      \
	pop   %r13;      \
	pop   %r12;      \
	pop   %r11;      \
	pop   %r10;      \
	pop   %r9;       \
	pop   %r8;       \
	pop   %rbx;      \
	pop   %rcx;      \
	pop   %rdx;      \
	pop   %rsi;      \
	pop   %rdi;      \
	pop   %rbp;      \
	pop   %rax;      \
	sysret;
/*
 *  The order in which registers are stored in the pt_regs structure
 */

#define PT_EBX     0
#define PT_ECX     1
#define PT_EDX     2
#define PT_ESI     3
#define PT_EDI     4
#define PT_EBP     5
#define PT_EAX     6
#define PT_GS      7
#define PT_FS      8
#define PT_ES      9
#define PT_DS      10

#define PT_TRAPNO  11
#define PT_ERR     12

#define PT_EIP     13
#define PT_CS      14
#define PT_EFLAGS  15
#define PT_ESP     16
#define PT_SS      17

#endif /* __ASSEMBLER__ */

#endif /* X86_ENTRY_H_ */
