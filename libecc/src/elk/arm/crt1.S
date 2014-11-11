#include <bits/errno.h>
#include "target.h"

/** Call a function only if it exists.
 * Act as if it returned 0 or NULL if it's not defined.
 * Preserve r0-r3 since they might be arguments.
 */
#define weak_call(name)           \
        .weak name              ; \
        ldr     r12, =name      ; \
        cmp     r12, #0         ; \
        moveq   r0, #0          ; \
        blxne   r12

        .section .text, "x"
        .align  4
/** This is the reset start point.
 * For ARM this is a set of eight vectors, four bytes apart.
 * Use the vector to load the pc from a nearby handler function
 * address.
 */
        .global _start
_start:
        ldr     pc, =reset              // Reset.
        ldr     pc, =undefined          // Undefined opcode.
        ldr     pc, =swi                // Syscall.
        ldr     pc, =prefetch           // Prefetch.
        ldr     pc, =abort              // Abort.
        nop                             // Reserved.
        ldr     pc, =irq                // Interrupt.
// The FIQ interrupt starts here.
// Enter the FIQ handler directly to save  cycles.
fiq:
        wfi
        b       fiq

        .text
/* Defined the size of the stacks.
 * Currently, only the SYS mode needs its own stack.
 */
#define Stack_FIQ   0           // Size of the FIQ stack.
#define Stack_IRQ   0           // Size of the IRQ stack.
#define Stack_SVC   0           // Size of the SVC stack.
#define Stack_ABT   0           // Size of the ABT stack.
#define Stack_UND   0           // Size of the UND stack.
#define Stack_SYS   (4096*4)    // Size of the SYS stack.

/** The reset entry point.
 */
reset:
        /* Set up the various stack pointers by setting the correct mode
        * and initializing the stack pointers.
        */
        msr     cpsr_c, #Mode_FIQ|I_bit|F_bit
        ldr     r0, =__mem_top          // Set up stack for FIQs.
        mov     sp, r0
        sub     r0, #Stack_FIQ
        msr     cpsr_c, #Mode_IRQ|I_bit|F_bit
        mov     sp, r0
        sub     r0, #Stack_IRQ
        msr     cpsr_c, #Mode_ABT|I_bit|F_bit
        mov     sp, r0
        sub     r0, #Stack_ABT
        msr     cpsr_c, #Mode_UND|I_bit|F_bit
        mov     sp, r0
        sub     r0, #Stack_UND
        msr     cpsr_c, #Mode_SVC|I_bit|F_bit
        mov     sp, r0
        sub     r0, #Stack_SVC
        // Finally, set up the SYS mode stack pointer and stay in SYS mode.
        msr     cpsr_c, #Mode_SYS       // |I_bit|F_bit
        mov     sp, r0
        sub     r0, #Stack_SYS
	mov	r8, r0			// Preserve the heap end.

        // Point to the vectors.
        ldr     r0, =_start
        mcr     p15, 0, r0, c12, c0, 0

        mov     fp, #0
        mov     lr, #0
	mov	a2, r8			// Get the end of the heap.
        ldr     a1,=args
	bic	sp, sp, #0xF		// Align the stack.
        bl      __elk_start             // Enter ELK.

done:
        wfi
        b       done

// main()'s pseudo arguments.
args:
	.word	1			// argc
argv:
        .word   name
        .word   0                       // End of argv.
        .word   0                       // End of envp.
        // Auxv
        .word   0

// RICH: TODO
undefined:
        b       undefined

// RICH: TODO
prefetch:
        b       prefetch

// RICH: TODO
abort:
        b       abort

#define SAVED_REGS r0-r12, lr   // Registers saved in thread context.
#define NUM_SAVED 14            // The number of saved registers.

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
        srsfd   sp!, #Mode_SYS          // Save LR_irq and SPSR_irq on the
                                        // sys stack.
        cpsid   i, #Mode_SYS            // Switch to sys mode.
        push    { SAVED_REGS }          // Store other registers.
        weak_call(__elk_enter_irq)      // Enter the IRQ state.
        cmp     r0, #0                  // Have a context pointer?
        strne   sp, [r0]                // Save the context if so.
        mrcne   p15, 0, r1, c13, c0, 3  // Get the TLS.
        strne   r1, [r0, #4]            // Save the TLS.
        and     r0, sp, #4              // Test stack alignment.
        sub     sp, sp, r0              // Adjust the stack if needed.
        push    { r0, r1 }              // Save the adjustment.
        weak_call(__elk_unlock_ready)   // Clear the ready lock.
        weak_call(__elk_identify_irq)   // Clear interrupt and get irq
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
        cpsie   i                       // Enable IRQ
        pop     { r0, r1 }              // Restore the stack adjustment.
        add     sp, sp, r0              // And adjust.
        weak_call(__elk_leave_irq)      // Leave the IRQ state.
        cmp     r0, #0                  // Leaving?
        ldrne   sp, [r0]                // Get the new stack pointer.
        ldrne   r1, [r0, #4]            // Get the new TLS.
        mcrne   p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(__elk_unlock_ready)   // Clear the ready lock.
        pop     { SAVED_REGS }          // Restore registers
        rfeia   sp!                     // And return.

/* Set up a context for execution.
 * int __elk_new_context(__elk_context **savearea,
 *                       void * (*entry)(void *), uint32_t psr,
 *                       void *ret, intptr_t arg)
 * On entry:
 * r0 points to the new context's save area, initialized to the top of
 *     the stack, aligned to 8 bytes.
 * r1 is the new context's start address
 * r2 is the new context's PSR.
 * r3 is the argument to the context.
 */
        .global __elk_new_context
__elk_new_context:
        ldr     r7, [r0]                // Get the new stack pointer.
        stmfd   r7!, {r1, r2}           // Save the new pc and PSR.
        sub     r7, #12 * 4             // Room for registers r2-r12, lr.
        mov     r1, r3                  // Get the first argument.
        stmfd   r7!, {r1-r2}            // Save the argument.
        str     r7, [r0]                // Update the stack pointer.
        bx      lr

/* Switch from one context to the next and unlock the queue.
 * void __elk_switch(__elk_context **to, __elk_context **from);
 * On entry:
 * The ready list is locked.
 * r0 points to the next save area.
 * r1 points to the current save area.
 */
        .global __elk_switch
__elk_switch:
        srsfd   sp!, #Mode_SYS          // Save LR_sys and SPSR_sys on
                                        // the sys stack.
        push    { SAVED_REGS }          // Store other registers.
        str     sp, [r1]                // Save the current frame.
        ldr     sp, [r0]                // Get the new stack pointer.
        ldr     r1, [r0, #4]            // Get the new TLS.
        mcr     p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(__elk_unlock_ready)   // Clear the ready lock.
        pop     { SAVED_REGS }          // Restore registers
        rfeia   sp!                     // And dispatch.

/* Switch from one context to the next and unlock the queue.
 * void __elk_switch_arg(int arg, __elk_context **to, __elk_context **from);
 * On entry:
 * The ready list is locked.
 * r0 is the tenative return value when the context is restarted.
 * r1 points to the next save area.
 * r2 points to the current save area.
 */
        .global __elk_switch_arg
__elk_switch_arg:
        srsfd   sp!, #Mode_SYS          // Save LR_sys and SPSR_sys on
                                        // the sys stack.
        push    { SAVED_REGS }          // Store other registers.
        str     sp, [r2]                // Save the current frame.
        ldr     sp, [r1]                // Get the new stack pointer.
        ldr     r1, [r1, #4]            // Get the new TLS.
        mcr     p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(__elk_unlock_ready)   // Clear the ready lock.
        pop     { SAVED_REGS }          // Restore registers
        rfeia   sp!                     // And dispatch.

/* Enter a context and unlock the queue.
 * void __elk_enter(__elk_context **new);
 * On entry:
 * The ready list is locked.
 * r0 points to the next save area.
 */
        .global __elk_enter
__elk_enter:
        srsfd   sp!, #Mode_SYS          // Save LR_sys and SPSR_sys on
                                        // the sys stack.
        ldr     sp, [r0]                // Get the new stack pointer.
        ldr     r1, [r0, #4]            // Get the new TLS.
        mcr     p15, 0, r1, c13, c0, 3  // Set the TLS.
        weak_call(__elk_unlock_ready)   // Clear the ready lock.
        pop     { SAVED_REGS }          // Restore registers
        rfeia   sp!                     // And dispatch.

/* Handle a system call.
 * On the ARM, this is entered with an SVC instruction.
 */
swi:
        srsfd   sp!, #Mode_SYS          // Save LR_svc and SPSR_svc on
                                        // the sys stack.
        cpsid   i, #Mode_SYS            // Switch to sys mode.
        push    { SAVED_REGS }          // Save registers.
        cpsie   i                       // Enable IRQ
        mov     r8, sp                  // Save the sp.
        push    { r0, r1 }
        weak_call(__elk_thread_self)    // Get the current context.
        cmp     r0, #0
        strne   r8, [r0]                // Save sp for clone().
        pop     { r0, r1 }

        // In a system call.
        // r7 is the call number.
__in_syscall:                           // The stack frame is valid here.
        cmp     r7, #512                // Is it a valid syscall number?
        ldr     r8, =syscall            // Point to the syscall save area.
        str     r7, [r8]                // Save call number for error
                                        // reporting.
        bhs     check_arm_syscall       // Jump if too big for a syscall.
        ldr     r8, =swi_table          // Get the system call table.
        ldr     r8, [r8, r7, lsl #2]    // Get the system call entry.
        ldr     r7, [sp, #4 * NUM_SAVED]// Get the SVC return address
                                        // for clone().
        push    { r4, r5, r6, r7 }      // Put extra arguments on the stack.
        blx     r8                      // Dispatch. Return value is in r0/r1
        add     sp, #6 * 4              // Deallocate args and r0-r1
                                        // from SAVED_REGS.
        pop     { r2-r12, lr }          // SAVED_REGS - r0-r1
        rfeia   sp!                     // And return.

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
        pop     { SAVED_REGS }          // Restore registers.
        rfeia   sp!                     // And return.

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
        weak_call(__elk_thread_self)    // Get the current context.
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
        push    { r0-r5 }
        weak_call(__elk_setup_console)  // Make sure the console is ready.
        cmp     r0, #0                  // Has the console been set up?
        pop     { r0-r5 }
        beq     2f                      // Jump if not.
        push    {r2-r5}                 // Push extra arguments.
        mov     r3, r1
        mov     r2, r0
        ldr     r1, =syscall            // And the system call number.
        ldr     r1, [r1]
        ldr     r0, =string             // Get the format string.
        weak_call(printf)               // Print if we have printf.
        add     sp, #16                 // clean up the stack.
2:
        mov     r0, #-ENOSYS
        ldmfd   sp!, {r12, pc}

// Set a system call entry.
// r0 = the entry number.
// r1 - the handling function.
        .global __elk_set_syscall
__elk_set_syscall:
        cmp     r0, #512
        ldrlo   r2, =swi_table          // Get the system call table.
        strlo   r1, [r2, r0, lsl #2]    // Set the stystem call entry.
        movhs   r0, #-1
        movlo   r0, #0
        bx      lr

string:
        .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
        .asciz "kernel"

        .data
        .global __heap_end__
__heap_end__:
        .word   0
syscall:
        .word   0

// Create the system call table with room for 512 entries.
swi_table:
        .rep    512
        .word   unhandled_swi
        .endr
