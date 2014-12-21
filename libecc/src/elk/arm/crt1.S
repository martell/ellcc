#include <bits/errno.h>
#include "config.h"
#include "asm.h"
#include "cpu.h"
#include "trap.h"
#include "syscall.h"
#include "context.h"

/* Context dump flags.
 */
#define CTX_UNDEFINED                   0x00000001
#define CTX_PREFETCH                    0x00000002
#define CTX_ABORT                       0x00000004
#define CTX_IRQ_ENTRY                   0x00000008
#define CTX_IRQ_EXIT                    0x00000010
#define CTX_NEW_CONTEXT                 0x00000020
#define CTX_SWITCH_CONTEXT_OUT          0x00000040
#define CTX_SWITCH_CONTEXT_IN           0x00000080
#define CTX_SWITCH_CONTEXT_ARG_OUT      0x00000100
#define CTX_SWITCH_CONTEXT_ARG_IN       0x00000200
#define CTX_SWI_ENTRY                   0x00000400
#define CTX_SWI_EXIT                    0x00000800
#define CTX_ENTER_CONTEXT               0x00001000

// Common debug sets.
#define CTX_IRQ         (CTX_IRQ_ENTRY|CTX_IRQ_EXIT)
#define CTX_SWI         (CTX_SWI_ENTRY|CTX_SWI_EXIT)

#define CTX_DEBUG 0     // CTX_IRQ|CTX_SWI
#define IN_RAM 1        // != 0 if debugging in RAM (better handling of bad sp).

#if CTX_DEBUG
#if IN_RAM
        .bss
        .comm   dump_stk, 4096

#define TRAP_DUMP(where)                                                \
        str     sp, saved_sp;                                           \
        ldr     sp, =dump_stk + 4096;                                   \
        push    { r0, r1, r2, r3, r4, r5, r6, r7, lr };                 \
        ldr     r1, saved_sp;                                           \
        ldr     r0, =where ## _str;                                     \
        .data;                                                          \
where ## _str:;                                                         \
        .asciz  #where;                                                 \
        .text;                                                          \
        bl      CLABEL(trap_dump);                                      \
        pop     { r0, r1, r2, r3, r4, r5, r6, r7, lr };                 \
        ldr     sp, saved_sp;

#else
// Not in RAM, nowhere to put the sp.
#define TRAP_DUMP(where)                                                \
        push    { r0, r1, r2, r3, r4, r5, r6, r7, lr };                 \
        mov     r1, sp;                                                 \
        add     r1, #4 * 9;                                             \
        ldr     r0, =where ## _str;                                     \
        .data;                                                          \
where ## _str:;                                                         \
        .asciz  #where;                                                 \
        .text;                                                          \
        bl      CLABEL(trap_dump);                                      \
        pop     { r0, r1, r2, r3, r4, r5, r6, r7, lr };
#endif  // IN_RAM

#define TRAP_DUMP_REG(reg, where)                                       \
        push    { r0, r1, r2, r3, r4, r5, r6, r7, lr };                 \
        mov     r1, reg;                                                \
        ldr     r0, =where ## _str;                                     \
        .data;                                                          \
where ## _str:;                                                         \
        .asciz  #where;                                                 \
        .text;                                                          \
        bl      CLABEL(trap_dump);                                      \
        pop     { r0, r1, r2, r3, r4, r5, r6, r7, lr };
#endif // CTX_DEBUG


/** Call a function only if it exists.
 * Act as if it returned 0 or NULL if it's not defined.
 * Preserve r0-r3 since they might be arguments.
 */
#define weak_call(name)           \
        .weak   name;             \
        ldr     r12, =name;       \
        cmp     r12, #0;          \
        moveq   r0, #0;           \
        blxne   r12;

        .section ".text","ax"
        .code 32
/** This is the reset start point.
 * For ARM this is a set of eight vectors, four bytes apart.
 * Use the vector to load the pc from a nearby handler function
 * address.
 */
        .global _start
_start:
        b       reset                   // Relative jump.
#if CTX_DEBUG && IN_RAM
saved_sp:
        .word   0
#endif

        .align  5
vector_start:
/* Exception vector
 *
 * This table will be copied to an appropriate location.
 * (the location is platform specific.)
 */
        ldr     pc, reset_p             // 0x00 mode: svc.
        ldr     pc, undefined_p         // 0x04 mode: und.
        ldr     pc, swi_p               // 0x08 mode: svc.
        ldr     pc, prefetch_p          // 0x0c mode: abort.
        ldr     pc, abort_p             // 0x10 mode: abort.
        nop                             // 0x14 reserved.
        ldr     pc, irq_p               // 0x18 mode: irq.
        ldr     pc, fiq_p               // 0x1c mode: fiq.

reset_p:        .word  reset
undefined_p:    .word  undefined
swi_p:          .word  swi
prefetch_p:     .word  prefetch
abort_p:        .word  abort
irq_p:          .word  irq
fiq_p:          .word  fiq

vector_end:

// The FIQ interrupt starts here.
fiq:
        wfi
        b       fiq

        .text

/** The reset entry point.
 */
reset:
        /* Set up the various stack pointers by setting the correct mode
        * and initializing the stack pointers.
        */
        msr     cpsr_c, #PSR_FIQ_MODE|PSR_IRQ_DIS|PSR_FIQ_DIS
        ldr     sp, =__intstk_top__
        msr     cpsr_c, #PSR_IRQ_MODE|PSR_IRQ_DIS|PSR_FIQ_DIS
        ldr     sp, =__intstk_top__
        msr     cpsr_c, #PSR_ABT_MODE|PSR_IRQ_DIS|PSR_FIQ_DIS
        ldr     sp, =__abortstk_top__
        msr     cpsr_c, #PSR_UND_MODE|PSR_IRQ_DIS|PSR_FIQ_DIS
        ldr     sp, =__undefinedstk_top__
        msr     cpsr_c, #PSR_SVC_MODE|PSR_IRQ_DIS|PSR_FIQ_DIS
        ldr     sp, =__svcstk_top__
        msr     cpsr_c, #PSR_SYS_MODE|PSR_IRQ_DIS|PSR_FIQ_DIS
        ldr     sp, =__sysstk_top__

        /** Finally, set up the krn mode stack pointer and stay in krn mode.
         * krn mode is a pseudo mode that kernel threads run in. It must
         * be different from svc mode to allow kernel threads to make
         * system calls. Except for a brief time during trap handling, all
         * ELK threads are either in krn mode or usr mode.
         */
        msr     cpsr_c, #PSR_KRN_MODE
        ldr     sp, =__sysstk_top__

        ldr     r0, =__mmu_enabled__    // Check for MMU enabled.
        cmp     r0, #0
        beq     nommu
        // Setup control register.
        mov     r0, #CTL_DEFAULT
        mcr     p15, 0, r0, c1, c0, 0

        /* Initialize page table
         * The physical address 0-4M is mapped on virtual address 2G.
         */

        // Clear the page directory */
        ldr     r1, =__boot_pgd_physical__
        ldr     r2, =(__boot_pgd_physical__ + 0x4000) // +16k
        mov     r0, #0
1:
        str     r0, [r1], #4
        teq     r1, r2
        bne     1b

        // Set PTE0 address in pgd. This sets up the map at 0x80000000.
        ldr     r1, =(__boot_pgd_physical__ + 0x2000)
        ldr     r0, =__boot_pte0_physical__
        orr     r0, r0, #0x01           // Used.
        str     r0, [r1], #4
        add     r0, #1024
        str     r0, [r1], #4
        add     r0, #1024
        str     r0, [r1], #4
        add     r0, #1024
        str     r0, [r1], #4

        ldr     r1, =__boot_pte0_physical__     // Fill boot page table entry.
        add     r2, r1, #0x1000
        ldr     r0, =(__syspage_physical_base__ + 0x1E)
1:
        str     r0, [r1], #4
        add     r0, r0, #0x1000
        teq     r1, r2
        bne     1b

        /* Enable paging.
         * The physical address __syspage_physical_base__ is temporarily mapped
         * to virtual address __syspage_physical_base__. This is needed to
         * enable paging.
         */
        ldr     r8, =__syspage_physical_base__
        lsr     r8, #18                 // Calculate the pgd table entry.
        ldr     r1, =__boot_pgd_physical__      // Set PTE0 address in pgd.
        add     r1, r1, r8              // Add the entry offset and save it.
        ldr     r0, =__boot_pte0_physical__
        orr     r0, r0, #0x01
        str     r0, [r1]

        mov     r0, #0
        mcr     p15, 0, r0, c7, c10, 4  // Drain write buffer.
        mcr     p15, 0, r0, c8, c7, 0   // flush I,D TLBs.
        ldr     r1, =__boot_pgd_physical__
        mcr     p15, 0, r1, c2, c0, 0   // Load the page table pointer.
        mov     r0, #-1
        mcr     p15, 0, r0, c3, c0      // Load the domain access register.
        mrc     p15, 0, r0, c1, c0, 0
enable_vm:
        orr     r0, r0, #0x1000         // I-cache enable.
        orr     r0, r0, #0x003d         // Write buffer, mmu.
        mcr     p15, 0, r0, c1, c0, 0

        // Reload PC register for virutal address.
        ldr     pc, =reload_pc          // Reset pc here.
reload_pc:
        /* Unmap 0-4M.
         * Since the first page must be accessible for exception
         * vector, we have to map it later.
         */
        ldr     r1, =__boot_pgd__       // Set PTE0 address in pgd.
        add     r1, r1, r8              // Using the previous offset.
        mov     r0, #0
        str     r0, [r1]
        mcr     p15, 0, r0, c8, c7, 0   // Flush I,D TLBs.
nommu:

        ldr     r0, =vector_start
        mcr     p15, 0, r0, c12, c0, 0

        mov     fp, #0
        mov     lr, #0
        ldr     a2, =__mem_top          // Get the end of the heap.
        ldr     a1,=args
        bic     sp, sp, #0xF            // Align the stack.
        bl      __elk_start             // Enter ELK.
done:
        wfi
        b       done

/** All threads are started in the kernel in sys mode.
 * This call puts a sys mode thread into usr mode.
 */
ENTRY(to_user)
        msr     cpsr_c, #PSR_USR_MODE
        mov     pc, lr

/** Wait for an intterupt to occur.
 */
ENTRY(suspend)
        wfi
        mov     pc, lr

/*
 * Relocate exception vector
 *
 * void vector_copy(vaddr_t dest);
 */
ENTRY(vector_copy)
        ldr     r1, =vector_start
        ldr     r2, =vector_end
1:
        ldmia   r1!, {r3}
        stmia   r0!, {r3}
        teq     r1, r2
        bne     1b
        mov     pc, lr

/** Enable cache
 */
ENTRY(cache_init)
  mov  pc, lr

// main()'s pseudo arguments.
args:
        .word  1                        // argc
argv:
        .word   name
        .word   0                       // End of argv.
        .word   0                       // End of envp.
        // Auxv
        .word   0

#define SAVED_REGS r0-r12, lr           // Registers saved in thread context.
#define NUM_SAVED 14                    // The number of saved registers.

/** Save the current context.
 * This is a little tricky because nested system calls and non-blocking
 * system calls from interrupt handlers are allowed.
 * The current thread state is saved on its system stack, see context.h.
 */
#define HASH #
#define SAVE_CONTEXT(mode, stack)                                       \
        ldr     sp, =stack;                                             \
        srsfd   sp!, HASH mode;         /* Save the spsr and lr. */     \
        cpsid   i, #PSR_KRN_MODE;       /* Switch back to krn mode. */  \
        sub     sp, #CTXREGS;           /* Allocate a context frame. */ \
        push    { r0, r1 };                                             \
        ldr     r0, =stack;                                             \
        ldr     r1, [r0, #-8];          /* Get the lr (return pc). */   \
        str     r1, [sp, #REG_PC + 8];  /* Put on the krn stack. */     \
        ldr     r1, [r0, #-4];          /* Get the spsr. */             \
        str     r1, [sp, #REG_CPSR + 8];/* Put on the krn stack. */     \
        and     r1, #PSR_MODE;          /* Get the previous mode. */    \
        cmp     r1, #PSR_KRN_MODE;      /* Were we in krn mode? */      \
        addeq   r1, sp, #8;             /* Point to the context. */     \
        beq     1f;                     /* Jump if so. */               \
        cmp     r1, #PSR_USR_MODE;      /* Were we in usr mode? */      \
        moveq   r1, #PSR_SYS_MODE;      /* If so, use sys mode. */      \
        orr     r1, #PSR_INT_MASK;      /* Set mode and disable ints. */\
        msr     cpsr_c, r1;             /* Switch to the previous. */   \
        mov     r1, sp;                 /* Copy the old sp. */          \
        mov     r0, lr;                 /* Copy the old lr. */          \
        cpsid   i, #PSR_KRN_MODE;       /* Switch back. */              \
        mov     lr, r0;                 /* Update the lr. */            \
1:      str     r1, [sp, #REG_SP + 8];  /* Save the old sp. */          \
        mrc     p15, 0, r0, c13, c0, 3; /* Get the tls. */              \
        str     r0, [sp, #REG_TLS + 8]; /* Save it. */                  \
        pop     { r0, r1 };                                             \
        add     sp, #4 * NUM_SAVED;     /* Prepare for the push. */     \
        push    { r0-r12, lr };         /* Save registers. */           \
        weak_call(CLABEL(thread_self))  /* Point to the context. */     \
        cmp     r0, #0;                                                 \
        strne   sp, [r0];               /* Save the context. */         \
        ldr     r0, [sp, #REG_R0];

#define RESTORE_CONTEXT()                                               \
        ldr     r0, [sp, #REG_TLS];     /* Get the tls. */              \
        mcr     p15, 0, r0, c13, c0, 3; /* Set the tls. */              \
        ldr     r0, [sp, #REG_CPSR];    /* Get the old mode. */         \
        and     r0, r0, #PSR_MODE;      /* Get the previous mode. */    \
        cmp     r0, #PSR_KRN_MODE;      /* Were we in krn mode? */      \
        ldreq   sp, [sp, #REG_SP];      /* Get the saved sp. */         \
        beq     1f;                     /* Jump if so. */               \
        cmp     r0, #PSR_USR_MODE;      /* Were we in usr mode? */      \
        moveq   r0, #PSR_SYS_MODE;      /* If so, use sys mode. */      \
        orr     r0, #PSR_INT_MASK;      /* Set mode and disable ints. */\
        ldr     r1, [sp, #REG_SP];      /* Get the saved sp. */         \
        ldr     r2, [sp, #REG_LR];      /* Get the saved lr. */         \
        msr     cpsr_c, r0;             /* Switch to the previous. */   \
        mov     sp, r1;                 /* Copy the old sp. */          \
        mov     lr, r2;                 /* Copy the old lr. */          \
        cpsid   i, #PSR_KRN_MODE;       /* Switch back. */              \
1:      pop     { r0-r12, lr };         /* Restore registers. */        \
        add     sp, #4 * 2;             /* Remove sp and tls. */        \
        rfeia   sp!;                    /* And return. */

/** Undefined instruction
 */
undefined:
        sub     lr, lr, #4              // Adjust return address.
        SAVE_CONTEXT(PSR_UND_MODE, __undefinedstk_top__)
#if CTX_DEBUG & CTX_UNDEFINED
        TRAP_DUMP(undefined);
#endif
        // RICH: cpsie   i                       // Enable IRQ.
        mov     r0, #TRAP_UNDEFINED
        mov     r1, sp
        weak_call(CLABEL(trap_handler))
        cmp     r0, #0
trapund:
        beq     trapund                 // Jump if no handler.
        RESTORE_CONTEXT()

/** Prefetch abort
 */
prefetch:
        sub     lr, lr, #8              // Adjust the lr.
        SAVE_CONTEXT(PSR_ABT_MODE, __abortstk_top__)
#if CTX_DEBUG & CTX_PREFETCH
        TRAP_DUMP(prefetch)
#endif
        // Remember to preserve r8 to save the context pointer.
        // RICH: cpsie   i                       // Enable IRQ.
        mov     r0, #TRAP_PREFETCH_ABORT
        mov     r1, sp
        weak_call(CLABEL(trap_handler))
        cmp     r0, #0
trappre:
        beq     trappre                 // Jump if no handler.
        RESTORE_CONTEXT()

/** Data abort
 */
abort:
        sub     lr, lr, #4              // Adjust return address.
        SAVE_CONTEXT(PSR_ABT_MODE, __abortstk_top__)
#if CTX_DEBUG & CTX_ABORT
        TRAP_DUMP(abort)
#endif
        // Remember to preserve r8 to save the context pointer.
        // RICH: cpsie   i                       // Enable IRQ.
        mov     r0, #TRAP_DATA_ABORT
        mov     r1, sp
        weak_call(CLABEL(trap_handler))
        cmp     r0, #0
trapabt:
        beq     trapabt                 // Jump if no handler.
        RESTORE_CONTEXT()

/** Handle an interrupt.
 * 1. Enter SYS mode and save register.
 * 2. Inform the kernel that we are in the IRQ state.
 * 3. Save the user thread context if necessary.
 * 4. Save the thread local storage.
 * 5. Make sure the stack is aligned.
 * 6. Unlock the ready data (locked by #2).
 * 7. Clear the interrupt and get the handler.
 * 8. Enable interrupts.
 * 9. Get the handler function and private data pointer.
 * 10. Call the interrupt handler with the private data pointer.
 * 11. Restore the previous stack alignment.
 * 12. Inform the kernel that we may be leaving the IRQ state.
 * 13. Restore the user thread context if necessary.
 */
irq:
        sub     lr, lr, #4              // Adjust return address.
        SAVE_CONTEXT(PSR_IRQ_MODE, __intstk_top__)
        ldr     r9, =curspl             // Set the interrupt priority level.
        ldr     r10, [r9]               // Save the old level.
        mov     r1, #15                 // Highest level.
        str     r1, [r9]                // Set the new level.
#if CTX_DEBUG & CTX_IRQ_ENTRY
        TRAP_DUMP(irq_entry)
#endif
        weak_call(CLABEL(enter_irq))    // Enter the IRQ state.
        weak_call(CLABEL(identify_irq)) // Clear interrupt and get irq
                                        // handler in r0.
        cmp     r0, #0
        beq     1f                      // Jump if no handler.
        ldr     r1, [r0]                // Get the handler function.
        ldr     r0, [r0, #4]            // And the private data pointer.
        cmp     r0, #0                  // Has private data?
        ldrne   r0, [r0]                // Get the pointer.
        cmp     r1, #0                  // Handler function?
        blxne   r1                      // Call it.
1:
        cpsid   i                       // Disable IRQ.
        weak_call(CLABEL(leave_irq))    // Leave the IRQ state.
        cmp     r0, #0                  // End of interrupts?
        beq     1f
        ldr     sp, [r0]                // Get the new sp.
        str     r10, [r9]               // Restore the old level.
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
1:
#if CTX_DEBUG & CTX_IRQ_EXIT
        TRAP_DUMP(irq_exit)
#endif
        RESTORE_CONTEXT()

/* Set up a context for execution.
 * int new_context(context_t **savearea,
 *                 void * (*entry)(void *), uint32_t psr,
 *                 void *ret, intptr_t arg)
 * On entry:
 * r0 points to the new context's save area, initialized to the top of
 *     the stack, aligned to 8 bytes.
 * r1 is the new context's start address
 * r2 is the new context's PSR.
 * r3 is the argument to the context.
 * [sp] is the stack pointer.
 * [sp #4] is the tls.
 */
ENTRY(new_context)
        ldr     r7, [r0]                // Get the new context pointer.
        sub     r7, #CTXREGS
        str     r1, [r7, #REG_PC]       // Save the new pc, psr, arg, and sp.
        str     r2, [r7, #REG_CPSR]
        str     r3, [r7, #REG_R0]
        ldr     r3, [sp]
        str     r3, [r7, #REG_SP]
        ldr     r3, [sp, #4]
        str     r3, [r7, #REG_TLS]
        str     r7, [r0]                // Update the stack pointer.
#if CTX_DEBUG & CTX_NEW_CONTEXT
        TRAP_DUMP_REG(r7, new_context)
#endif
        bx      lr

/* Switch from one context to the next and unlock the queue.
 * void switch_context(context_t **to, context_t **from);
 * On entry:
 * The ready list is locked.
 * r0 points to the next save area.
 * r1 points to the current save area.
 */
ENTRY(switch_context)
        mrs     r2, cpsr                // Get the cpsr.
        push    { r2 }
        push    { lr }
        sub     sp, #4 * 2              // Room for the saved sp and tls.
        push    { SAVED_REGS }          // Store other registers.
        str     sp, [sp, #REG_SP]       // Save the sp.
        mrc     p15, 0, r2, c13, c0, 3  // Get the TLS.
        str     r2, [sp, #REG_TLS]      // Save the TLS.
        str     sp, [r1]                // Save the current frame.
#if CTX_DEBUG & CTX_SWITCH_CONTEXT_OUT
        TRAP_DUMP(switch_context_out)
#endif
        ldr     sp, [r0]                // Get the new context pointer.
#if CTX_DEBUG & CTX_SWITCH_CONTEXT_IN
        TRAP_DUMP(switch_context_in)
#endif
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        RESTORE_CONTEXT()

/* Switch from one context to the next and unlock the queue.
 * void switch_context_arg(int arg, context_t **to, context_t **from);
 * On entry:
 * The ready list is locked.
 * r0 is the tenative return value when the context is restarted.
 * r1 points to the next save area.
 * r2 points to the current save area.
 */
ENTRY(switch_context_arg)
        mrs     r3, cpsr                // Get the cpsr.
        push    { r3 }
        push    { lr }
        sub     sp, #4 * 2              // Room for the saved sp and tls.
        push    { SAVED_REGS }          // Store other registers.
        str     sp, [sp, #REG_SP]       // Save the sp.
        mrc     p15, 0, r3, c13, c0, 3  // Get the TLS.
        str     r3, [sp, #REG_TLS]      // Save the TLS.
        str     sp, [r2]                // Save the current frame.
#if CTX_DEBUG & CTX_SWITCH_CONTEXT_ARG_OUT
        TRAP_DUMP(switch_context_arg_out)
#endif
        ldr     sp, [r1]                // Get the new context pointer.
#if CTX_DEBUG & CTX_SWITCH_CONTEXT_ARG_IN
        TRAP_DUMP(switch_context_arg_in)
#endif
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        RESTORE_CONTEXT()

/* Enter a context and unlock the queue.
 * On entry:
 * The ready list is locked.
 * r0 is an argument to the cleanup function.
 * r1 is an optional cleanup function to call or NULL.
 * r2 points to the next save area.
 */
ENTRY(enter_context)
        mov     sp, r2                  // Get the new stack pointer.
        cmp     r1, #0                  // Need a cleanup call?
        blxne   r1                      // Call it if so.
#if CTX_DEBUG & CTX_ENTER_CONTEXT
        TRAP_DUMP(enter_context)
#endif
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        RESTORE_CONTEXT()

/* Handle a system call.
 * On the ARM, this is entered with an SVC instruction.
 */
swi:
        SAVE_CONTEXT(PSR_SVC_MODE, __svcstk_top__)
#if CTX_DEBUG & CTX_SWI_ENTRY
        TRAP_DUMP(swi_entry)
#endif
// RICH:        cpsie   i                       // Enable IRQ.
        ldr     r10, [sp, #REG_PC]      // Get the return address for clone().
__in_syscall:                           // The stack frame is valid here.
        ldr     r9, =syscall            // Point to the syscall save area.
        str     r7, [r9]                // Save call number for error
                                        // reporting.
        cmp     r7, #512                // Is it a valid syscall number?
        bhs     check_arm_syscall       // Jump if too big for a syscall.
        // RICH: Get rid of stack adjustment: System stacks are aligned.
        and     r9, sp, #4              // Test stack alignment.
        sub     sp, sp, r9              // Adjust the stack if needed.
        push    { r8, r9 }              // Save the adjustment.
        ldr     r8, =swi_table          // Get the system call table.
        ldr     r8, [r8, r7, lsl #2]    // Get the system call entry.
        // RICH: Check this.
        mov     r7, r10                 // Get the SVC return address
                                        // for clone().
        push    { r4, r5, r6, r7 }      // Put extra arguments on the stack.
        blx     r8                      // Dispatch. Return value is in r0/r1
        add     sp, #4 * 4              // Deallocate args.
        pop     { r8, r9 }              // Restore the stack adjustment and lr.
        add     sp, sp, r9              // And adjust.
        str     r0, [sp, #REG_R0]       // Save possible return values.
        str     r1, [sp, #REG_R1]       // Save possible return values.
#if CTX_DEBUG & CTX_SWI_EXIT
        TRAP_DUMP(swi_exit)
#endif
        RESTORE_CONTEXT()

/* ARM has a few "hidden" system calls.
 * Only one of these is used by musl and is defined below for
 * access from kernel space.
 */
check_arm_syscall:
        ldr     r9, low
        cmp     r7, r9
        blo     unhandled_swi           // RICH: Incorrect
        ldr     r9, high
        cmp     r7, r9
        bhi     unhandled_swi
        and     r7, r7, #0x7
        adr     r9, ARM_SVC_Table       // Get the system call table.
        ldr     r7, [r9, r7, lsl #2]    // Get the stystem call entry.
        blx     r7                      // Dispatch. Return value is in r0/r1
        RESTORE_CONTEXT()

low:
        .word   0xF0000
high:
        .word   0xF0007

ARM_SVC_Table:
        .word   unhandled_swi
        .word   unhandled_swi
        .word   unhandled_swi
        .word   unhandled_swi
        .word   unhandled_swi
        .word   __set_thread_area
        .word   unhandled_swi
        .word   unhandled_swi

/* Set thread area.
 * This definitions overrides the musl definition in __set_thread_area.s.
 * It is overridden to eliminate an unnecessary system call in kernel space.
 */
        .global __set_thread_area
__set_thread_area:
        mcr     p15, 0, r0, c13, c0, 3
        mov     r0, #0
        bx      lr

/* Get thread area.
 * This definitions overrides the musl definition in tls.s.
 */
        .global __aeabi_read_tp
__aeabi_read_tp:
        mrc     p15, 0, r0, c13, c0, 3
        bx      lr

// Unhandled system calls.
unhandled_swi:
        stmfd   sp!, {r12, lr}
        push    {r2-r5}                 // Push extra arguments.
        mov     r3, r1
        mov     r2, r0
        ldr     r1, =syscall            // And the system call number.
        ldr     r1, [r1]
        ldr     r0, =unhandled_string   // Get the format string.
        weak_call(diag_printf)          // Print the unhandled system call.
        add     sp, #16                 // clean up the stack.
        mov     r0, #-ENOSYS
        ldmfd   sp!, {r12, pc}

// Set a system call entry.
// r0 = the entry number.
// r1 - the handling function.
ENTRY(set_syscall)
        cmp     r0, #512
        ldrlo   r2, =swi_table          // Get the system call table.
        strlo   r1, [r2, r0, lsl #2]    // Set the stystem call entry.
        movhs   r0, #-1
        movlo   r0, #0
        bx      lr

/** Copy data from user to kernel space.
 * Returns 0 on success, or EFAULT on page fault.
 *
 *  int copyin(const void *uaddr, void *kaddr, size_t len)
 */
        .global ASM_LABEL(known_fault1)
ENTRY(copyin)
        mov     r12, sp
        stmdb   sp!, {r4, r11, r12, lr, pc}
        sub     r11, r12, #4
        mov     r12, #0
        b       2f
1:
        ldrb    r3, [r12, r0]
ASM_LABEL(known_fault1):                // Might fault here.
        strb    r3, [r12, r1]
        add     r12, r12, #1
2:
        subs    r2, r2, #1
        bcs     1b
        mov     r0, #0                  // Set no error.
        ldmia   sp, {r4, r11, sp, pc}

/** Copy data to user from kernel space.
 * Returns 0 on success, or EFAULT on page fault.
 *
 *  int copyout(const void *kaddr, void *uaddr, size_t len)
 */
        .global ASM_LABEL(known_fault2)
ENTRY(copyout)
        mov     r12, sp
        stmdb   sp!, {r4, r11, r12, lr, pc}
        sub     r11, r12, #4
        mov     r12, #0
        b       2f
1:
        ldrb    r3, [r12, r0]
ASM_LABEL(known_fault2):                // Might fault here.
        strb    r3, [r12, r1]
        add     r12, r12, #1
2:
        subs    r2, r2, #1
        bcs     1b
        mov     r0, #0      /* Set no error */
        ldmia   sp, {r4, r11, sp, pc}

/*
 * copyinstr - Copy string from user space.
 * Returns 0 on success, or EFAULT on page fault, or ENAMETOOLONG.
 *
 *  syntax - int copyinstr(const char *uaddr, void *kaddr, size_t len);
 */
        .global ASM_LABEL(known_fault3)
ENTRY(copyinstr)
        mov     r12, sp
        stmdb   sp!, {r4, r11, r12, lr, pc}
        sub     r11, r12, #4
        mov     r12, #0
        b       2f
1:
        ldrb    r3, [r12, r0]
ASM_LABEL(known_fault3):                // Might fault here.
        strb    r3, [r12, r1]
        cmp     r3, #0
        beq     3f
        add     r12, r12, #1
2:
        subs    r2, r2, #1
        bcs     1b
        mov     r0, #(-ENAMETOOLONG)
        b       4f
3:
        mov     r0, #0      /* Set no error */
4:
        ldmia   sp, {r4, r11, sp, pc}

/** Fault entry for user access
 */
ENTRY(copy_fault)
	mov	r0, #(-EFAULT)
 	ldmia	sp, {r4, r11, sp, pc}

/** int spl0(void);
 */
ENTRY(spl0)
        ldr     r1, =curspl             // curspl = 0.
        ldr     r0, [r1]
        mov     r2, #0
        str     r2, [r1]

        mrs     r1, cpsr                // Enable interrupt.
        bic     r1, r1, #0xc0
        msr     cpsr_c, r1
        mov     pc, lr

/** int splhigh(void);
 */
ENTRY(splhigh)
        mrs     r1, cpsr                // Disable interrupt.
        orr     r1, r1, #0xc0
        msr     cpsr_c, r1

        ldr     r1, =curspl             // curspl = 15.
        ldr     r0, [r1]
        mov     r2, #15
        str     r2, [r1]
        mov     pc, lr

/** void splx(int s);
 */
ENTRY(splx)
        mov     r3, r0                  // r3: new spl.
        ldr     r1, =curspl
        ldr     r0, [r1]
        cmp     r3, r0
        moveq   pc, lr                  // Return if equal
        str     r3, [r1]

        cmp     r3, #0
        mrs     r1, cpsr
        bic     r1, r1, #0xc0
        orrne   r1, r1, #0xc0           // Disable interrupt if curspl != 0.
        msr     cpsr_c, r1
        mov     pc, lr

/** void sploff(void);
 */
ENTRY(sploff)
        mrs     r0, cpsr
        orr     r0, r0, #0xc0
        msr     cpsr_c, r0
        mov     pc, lr

/** void splon(void);
 */
ENTRY(splon)
        mrs     r0, cpsr
        bic     r0, r0, #0xc0
        msr     cpsr_c, r0
        mov     pc, lr

unhandled_string:
        .asciz  "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
        .asciz  "kernel"

        .data
        .global __heap_end__
__heap_end__:
        .word   0
syscall:
        .word   0

// Current spl.
curspl:
        .word   0

// Create the system call table with room for 512 entries.
swi_table:
        .rep    512
        .word   unhandled_swi
        .endr

