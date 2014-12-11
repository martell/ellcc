#include <bits/errno.h>
#include "config.h"
#include "asm.h"
#include "cpu.h"
#include "trap.h"
#include "syscall.h"
#include "context.h"

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

        .align  5
vector_start:
/* Exception vector
 *
 * This table will be copied to an appropriate location.
 * (the location is platform specific.)
 */
        ldr     pc, reset_p             // 0x00 mode: svc.
        ldr     pc, undefined_p         // 0x04 mode: ?.
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
        // Finally, set up the SYS mode stack pointer and stay in SYS mode.
        msr     cpsr_c, #PSR_SYS_MODE       // |PSR_IRQ_DIS|PSR_FIQ_DIS
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
 * This is a little tricky because nested system calls are allowed.
 */
#define HASH #
// RICH: current access has to be fixed for SMP. Need current[processor];
#define SAVE_CONTEXT(mode)                                              \
        ldr     sp, =CLABEL(current);   /* Get the current thread. */   \
        ldr     sp, [sp];               /* Indirectly.             */   \
        ldr     sp, [sp];               /* Get the system context. */   \
        srsfd   sp!, HASH mode;         /* Save saved lr and spsr */    \
                                        /* on the stack. */             \
        sub     sp, #4 * 2;             /* Leave room for the sp. */    \
        push    { SAVED_REGS };         /* Store other registers. */    \
        push    { r0-r1 };              /* Preserve registers. */       \
        ldr     r0, =CLABEL(current);   /* Get the current thread. */   \
        ldr     r0, [r0];               /* Indirectly.             */   \
        add     r1, sp, #8;             /* Adjust the stack. */         \
        str     r1, [r0];               /* Save the new context. */     \
        ldr     r0, [sp, #REG_CPSR + 8];/* Get the saved psr. */        \
        and     r0, #PSR_MODE;          /* Get the old mode. */         \
        cmp     r0, #PSR_USR_MODE;      /* Were we in usr mode? */      \
        moveq   r0, #PSR_SYS_MODE;      /* If so, use sys mode. */      \
        orr     r0, #PSR_INT_MASK;      /* Set mode and disable ints. */\
        msr     cpsr_c, r0;             /* Switch to the previous. */   \
        mov     r0, sp;                 /* Copy the usr sp. */          \
        mov     r1, lr;                 /* Copy the usr lr. */          \
        cpsid   i, HASH mode;           /* Switch back. */              \
        str     r0, [sp, #REG_SP + 8];  /* Save the usr sp. */          \
        str     r1, [sp, #REG_LR + 8];  /* Save the usr lr. */          \
        pop     { r0-r1 };              /* Restore registers. */

/** Restore current context.
 */
#define RESTORE_CONTEXT(mode)                                           \
        ldr     r0, [sp, #REG_SP];      /* Get the saved sp. */         \
        ldr     r1, [sp, #REG_CPSR];    /* Get the saved psr. */        \
        and     r1, #PSR_MODE;          /* Get the old mode. */         \
        cmp     r1, #PSR_USR_MODE;      /* Were we in usr mode? */      \
        moveq   r1, #PSR_SYS_MODE;      /* If so, use sys mode. */      \
        orr     r1, #PSR_INT_MASK;      /* Set mode and disable ints. */\
        msr     cpsr_c, r1;             /* Switch to the previous. */   \
        mov     sp, r0;                 /* Set up the stack pointer. */ \
        cpsid   i, HASH mode;           /* Switch to previous mode. */  \
        ldr     r0, =CLABEL(current);   /* Get the current thread. */   \
        ldr     r0, [r0];               /* Indirectly.             */   \
        add     r1, sp, #CTXREGS;       /* Save the updated context. */ \
        str     r1, [r0];               /* Save the context pointer. */ \
        pop     { SAVED_REGS };         /* Restore registers. */        \
        add     sp, #4 * 2;             /* Remove room for the sp. */   \
        rfeia   sp!                     /* And return. */

/** Undefined instruction
 */
undefined:
        sub     lr, lr, #4              // Adjust return address.
        SAVE_CONTEXT(PSR_UND_MODE)      // Save the context.
        cpsie   i                       // Enable IRQ.
        mov     r0, #TRAP_UNDEFINED
        mov     r1, sp
        weak_call(CLABEL(trap_handler))
        cmp     r0, #0
trapund:
        beq     trapund                 // Jump if no handler.
        RESTORE_CONTEXT(PSR_UND_MODE)

/** Prefetch abort
 */
prefetch:
        sub     lr, lr, #8              // Adjust the lr.
        SAVE_CONTEXT(PSR_ABT_MODE)      // Save the context.
        cpsie   i                       // Enable IRQ.
        mov     r0, #TRAP_PREFETCH_ABORT
        mov     r1, sp
        weak_call(CLABEL(trap_handler))
        cmp     r0, #0
trappre:
        beq     trappre                 // Jump if no handler.
        RESTORE_CONTEXT(PSR_ABT_MODE)

/** Data abort
 */
abort:
        sub     lr, lr, #4              // Adjust return address.
        SAVE_CONTEXT(PSR_ABT_MODE)      // Save the context.
        cpsie   i                       // Enable IRQ.
        mov     r0, #TRAP_DATA_ABORT
        mov     r1, sp
        weak_call(CLABEL(trap_handler))
        cmp     r0, #0
trapabt:
        beq     trapabt                 // Jump if no handler.
        RESTORE_CONTEXT(PSR_ABT_MODE)

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
        srsfd   sp!, #PSR_IRQ_MODE      // Save lr and spsr on the
                                        // irq stack.
        sub     sp, #4 * 2              // Leave room for the sp.
        push    { SAVED_REGS }          // Store other registers.

        cpsid   i, #PSR_SYS_MODE        // Switch to sys mode.
        mov     r0, sp                  // Set the usr sp.
        cpsid   i, #PSR_IRQ_MODE        // Switch to previous mode.
        str     r0, [sp, #REG_SP]       // Save the user sp.
        bl      ASM_LABEL(splhigh)      // Set the interrupt level.
        mov     r8, r0                  // Save the current level.
        weak_call(CLABEL(enter_irq))    // Enter the IRQ state.
        cmp     r0, #0                  // Have a context pointer?
        strne   sp, [r0]                // Save the context if so.
        mrcne   p15, 0, r1, c13, c0, 3  // Get the TLS.
        strne   r1, [r0, #4]            // Save the TLS.
        and     r0, sp, #4              // Test stack alignment.
        sub     sp, sp, r0              // Adjust the stack if needed.
        push    { r0, r1 }              // Save the adjustment.
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
        // mov     r0, r8                  // Get interrupt level.
        // bl      ASM_LABEL(splx)         // Restore interrupt level.
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        pop     { r0, r1 }              // Restore the stack adjustment.
        add     sp, sp, r0              // And adjust.
        weak_call(CLABEL(leave_irq))    // Leave the IRQ state.
        cpsid   i                       // Disable IRQ
        cmp     r0, #0                  // Leaving?
        ldrne   sp, [r0]                // Get the new stack pointer.
        ldrne   r1, [r0, #4]            // Get the new TLS.
        mcrne   p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        ldr     r0, [sp, #REG_SP]       // Get the user sp.
        cpsid   i, #PSR_SYS_MODE        // Switch to sys mode.
        mov     sp, r0                  // Set the usr sp.
        cpsid   i, #PSR_IRQ_MODE        // Switch to previous mode.
        pop     { SAVED_REGS }          // Restore registers
        add     sp, #4 * 2              // Deallocate the sp space.
        rfeia   sp!                     // And return.

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
 * r4 is the usr stack pointer.
 */
ENTRY(new_context)
        ldr     r7, [r0]                // Get the new stack pointer.
        sub     r7, #CTXREGS
        str     r1, [r7, #REG_PC]       // Save the new pc, psr, arg, and sp.
        str     r2, [r7, #REG_CPSR]
        str     r3, [r7, #REG_R0]
        str     r4, [r7, #REG_SP]
        str     r7, [r0]                // Update the stack pointer.
        bx      lr

/* Switch from one context to the next and unlock the queue.
 * void switch_context(context_t **to, context_t **from);
 * On entry:
 * The ready list is locked.
 * r0 points to the next save area.
 * r1 points to the current save area.
 */
ENTRY(switch_context)
        srsfd   sp!, #PSR_SVC_MODE      // Save lr and spsr on the svc stack.
        sub     sp, #4 * 2              // Room for the saved sp
        push    { SAVED_REGS }          // Store other registers.
        str     sp, [r1]                // Save the current frame.
        ldr     sp, [r0]                // Get the new stack pointer.
        ldr     r1, [r0, #4]            // Get the new TLS.
        mcr     p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        RESTORE_CONTEXT(PSR_SVC_MODE)

/* Switch from one context to the next and unlock the queue.
 * void switch_context_arg(int arg, context_t **to, context_t **from);
 * On entry:
 * The ready list is locked.
 * r0 is the tenative return value when the context is restarted.
 * r1 points to the next save area.
 * r2 points to the current save area.
 */
ENTRY(switch_context_arg)
        srsfd   sp!, #PSR_SVC_MODE      // Save lr and spsr on the svc stack.
        sub     sp, #4 * 2              // Room for the saved sp
        push    { SAVED_REGS }          // Store other registers.
        str     sp, [r2]                // Save the current frame.
        ldr     sp, [r1]                // Get the new stack pointer.
        str     r0, [sp, #REG_R0]       // Save the return value.
        ldr     r1, [r1, #4]            // Get the new TLS.
        mcr     p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        RESTORE_CONTEXT(PSR_SVC_MODE)

/* Enter a context and unlock the queue.
 * void enter_context(context_t **new);
 * On entry:
 * The ready list is locked.
 * r0 points to the next save area.
 */
ENTRY(enter_context)
        ldr     sp, [r0]                // Get the new stack pointer.
        ldr     r1, [r0, #4]            // Get the new TLS.
        mcr     p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(CLABEL(unlock_ready)) // Clear the ready lock.
        ldr     r0, [sp, #REG_SP]       // Get the user sp.
        cpsid   i, #PSR_SYS_MODE        // Switch to sys mode.
        mov     sp, r0;                 // Set the usr sp.
        cpsid   i, #PSR_SVC_MODE        // Switch to previous mode.
        pop     { SAVED_REGS }          // Restore registers
        add     sp, #4 * 2              // Deallocate the saved sp
        rfeia   sp!                     // And dispatch.

/* Handle a system call.
 * On the ARM, this is entered with an SVC instruction.
 */
swi:
        SAVE_CONTEXT(PSR_SVC_MODE)      // Save the context.
        cpsie   i                       // Enable IRQ.

        // In a system call.
        // r7 is the call number.
__in_syscall:                           // The stack frame is valid here.
        cmp     r7, #512                // Is it a valid syscall number?
        ldr     r8, =syscall            // Point to the syscall save area.
        str     r7, [r8]                // Save call number for error
                                        // reporting.
        bhs     check_arm_syscall       // Jump if too big for a syscall.
        and     r8, sp, #4              // Test stack alignment.
        sub     sp, sp, r8              // Adjust the stack if needed.
        push    { r8, r9 }              // Save the adjustment.
        ldr     r8, =swi_table          // Get the system call table.
        ldr     r8, [r8, r7, lsl #2]    // Get the system call entry.
        ldr     r7, [sp, #4 * (NUM_SAVED + 4)]// Get the SVC return address
                                        // for clone().
        push    { r4, r5, r6, r7 }      // Put extra arguments on the stack.
        blx     r8                      // Dispatch. Return value is in r0/r1
        add     sp, #4 * 4              // Deallocate args.
        pop     { r8, r9 }              // Restore the stack adjustment.
        add     sp, sp, r8              // And adjust.
        str     r0, [sp, #REG_R0]       // Save possible return values.
        str     r1, [sp, #REG_R1]
        RESTORE_CONTEXT(PSR_SVC_MODE)

/* ARM has a few "hidden" system calls.
 * Only one of these is used by musl and is defined below for
 * access from kernel space.
 */
check_arm_syscall:
        ldr     r8, low
        cmp     r7, r8
        blo     unhandled_swi           // RICH: Incorrect
        ldr     r8, high
        cmp     r7, r8
        bhi     unhandled_swi
        and     r7, r7, #0x7
        adr     r8, ARM_SVC_Table       // Get the system call table.
        ldr     r7, [r8, r7, lsl #2]    // Get the stystem call entry.
        blx     r7                      // Dispatch. Return value is in r0/r1
        RESTORE_CONTEXT(PSR_SVC_MODE)
#if RICH
        pop     { SAVED_REGS }          // Restore registers.
        add     sp, #8                  // Deallocate saved sp.
        rfeia   sp!                     // And return.
#endif

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
        push    { r1, r11, r12, lr }
        mcr     p15, 0, r0, c13, c0, 3
        mov     r1, r0
        weak_call(CLABEL(thread_self))  // Get the current context.
        cmp     r0, #0                  // Have a context?
        strne   r1, [r0, #4]            // Save the TLS in the current Thread.
        movne   r0, #0
        moveq   r0, #-1
        pop     { r1, r11, r12, lr }
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
CLABEL(known_fault1):                   // Might fault here.
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
CLABEL(known_fault2):                   // Might fault here.
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
CLABEL(known_fault3):                   // Might fault here.
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

abort_string:
        .asciz  "in kernal abort trap\n"
prefetch_string:
        .asciz  "in kernal prefetch trap\n"
undefined_string:
        .asciz  "in kernal undefined trap\n"
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
        .word  0

// Create the system call table with room for 512 entries.
swi_table:
        .rep    512
        .word   unhandled_swi
        .endr
