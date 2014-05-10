#include <bits/errno.h>
#include "target.h"

    .section .text, "x"
    .align  4
    .global _start
_start:
    ldr     pc, reset_p
    ldr     pc, undefined_p
    ldr     pc, swi_p
    ldr     pc, prefetch_p
    ldr     pc, abort_p
    nop                         // Reserved. 
    ldr     pc, irq_p
// The FIQ interrupt starts here.
    b       .

reset_p:
    .word reset
undefined_p:
    .word undefined
swi_p:
    .word swi
prefetch_p:
    .word prefetch
abort_p:
    .word abort
irq_p:
    .word irq

    .text
/* Defined the size of the stacks.
 * Currently, only the SYS mode needs its own stack.
 */
#define Stack_FIQ   0           // Size of the FIQ stack.
#define Stack_IRQ   0           // Size of the IRQ stack.
#define Stack_SVC   0           // Size of the SVC stack.
#define Stack_ABT   0           // Size of the ABT stack.
#define Stack_UND   0           // Size of the UND stack.
#define Stack_SYS   4096        // Size of the SYS stack.

/** The reset entry point.
 */
reset:
    /* Set up the various stack pointers by setting the correct mode
     * and initializing the stack pointers.
     */
    msr     cpsr_c, #Mode_FIQ|I_bit|F_bit
    ldr     r0, =__mem_top           // Set up stack for FIQs.
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
    msr     cpsr_c, #Mode_SYS // |I_bit|F_bit
    mov     sp, r0
    sub     r0, #Stack_SYS
    // Set up the end of the heap.
    ldr     r1, heap_end_p
    str     r0, [r1]

    // Point to the vectors.
    ldr     r0, =_start
    mcr     p15, 0, r0, c12, c0, 0

    // Clear the bss area.
    ldr     a1, bss_start
    mov     a2, #0
    ldr     a3, bss_end
    sub     a3, a3, a1
    bl      memset

    // Now start up the system by initializing the library and calling main.
    mov     fp, #0
    mov     lr, #0
    str     fp, [sp,#-4]!
    str     a1, [sp,#-4]!
    adr     a3, argv
    mov     a2, #1
    ldr     a1,=main
    bl      __libc_start_main
    b       .

heap_end_p:
    .word   __heap_end
bss_start:
    .word   __bss_start__
bss_end:
    .word   __bss_end__

// main()'s pseudo arguments.
argv:
    .word   name
    .word   0                   // End of argv.
    .word   0                   // End of envp.
    // Auxv
    .word   0

undefined:
    b   .

prefetch:
    b   .

abort:
    b   .

fiq:
    b   .

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
    sub     lr, lr, #4          // Adjust return address.
    srsfd   sp!, #Mode_SYS      // Save LR_irq and SPSR_irq on the sys stack.
    cpsid   i, #Mode_SYS        // Switch to sys mode.
    push    { SAVED_REGS }      // Store other registers.
    bl      __enter_irq         // Enter the IRQ state.
    cmp     r0, #0              // Have a context pointer?
    strne   sp, [r0]            // Save the context if so.
    mrcne   p15, 0, r1, c13, c0, 3  // Get the TLS.
    strne   r1, [r0, #4]        // Save the TLS.
    and     r0, sp, #4          // Test stack alignment.
    sub     sp, sp, r0          // Adjust the stack if needed.
    push    { r0, r1 }          // Save the adjustment.
    bl      __unlock_ready      // Clear the ready lock.
    bl      __identify_irq      // Clear interrupt and get irq handler in r0.
    cpsie   i                   // Enable IRQ
    ldr     r1, [r0]            // Get the handler function.
    ldr     r0, [r0, #4]        // And the private data pointer.
    cmp     r0, #0              // Has private data?
    ldrne   r0, [r0]            // Get the pointer.
    cmp     r1, #0              // Handler function?
    blxne   r1                  // Call it.
    pop     { r0, r1 }          // Restore the stack adjustment.
    add     sp, sp, r0          // And adjust.
    bl      __leave_irq         // Leave the IRQ state.
    cmp     r0, #0              // Leaving?
    ldrne   sp, [r0]            // Get the new stack pointer.
    ldrne   r1, [r0, #4]        // Get the new TLS.
    mcrne   p15, 0, r1, c13, c0, 3 // Set the TLS.
    bl      __unlock_ready      // Clear the ready lock.
    pop     { SAVED_REGS }      // Restore registers
    rfeia   sp!                 // And return.

/* Set up a context for execution.
 * int __new_context(Context **savearea,
 *                   int (*entry)(intptr_t, intptr_t), uint32_t psr,
 *                   void *ret, intptr_t arg1, intptr_t arg2)
 * On entry:
 * r0 points to the new context's save area, initialized to the top of the stack, aligned to 8 bytes.
 * r1 is the new context's start address
 * r2 is the new context's PSR.
 * r3 is the first argument to the context.
 * [sp] is the second argument to the context.
 */
    .global __new_context
__new_context:
    ldr     r7, [r0]            // Get the new stack pointer.
    stmfd   r7!, {r1, r2}       // Save the new pc and PSR.
    sub     r7, #12 * 4         // Room for registers r2-r12, lr.
    mov     r1, r3              // Get the first argument.
    ldr     r2, [sp]            // Get the second argument.
    stmfd   r7!, {r1-r2}        // Save the arguments as r0, r1.
    str     r7, [r0]            // Update the stack pointer.
    bx      lr


/* Switch from one context to the next and unlock the queue.
 * void __switch(Context **to, Context **from);
 * On entry:
 * The ready list is locked.
 * r0 points to the next save area.
 * r1 points to the current save area.
 */
    .global __switch
__switch:
    srsfd   sp!, #Mode_SYS      // Save LR_sys and SPSR_sys on the sys stack.
    push    { SAVED_REGS }      // Store other registers.
    str     sp, [r1]            // Save the current frame.
    ldr     sp, [r0]            // Get the new stack pointer.
    ldr     r1, [r0, #4]        // Get the new TLS.
    mcr     p15, 0, r1, c13, c0, 3      // Set the TLS.
    bl      __unlock_ready      // Unlock the ready list.
    pop     { SAVED_REGS }      // Restore registers
    rfeia   sp!                 // And dispatch.

/* Switch from one context to the next and unlock the queue.
 * void __switch_arg(int arg, Context **to, Context **from);
 * On entry:
 * The ready list is locked.
 * r0 is the tenative return value when the context is restarted.
 * r1 points to the next save area.
 * r2 points to the current save area.
 */
    .global __switch_arg
__switch_arg:
    srsfd   sp!, #Mode_SYS      // Save LR_sys and SPSR_sys on the sys stack.
    push    { SAVED_REGS }      // Store other registers.
    str     sp, [r2]            // Save the current frame.
    ldr     sp, [r1]            // Get the new stack pointer.
    ldr     r1, [r1, #4]        // Get the new TLS.
    mcr     p15, 0, r1, c13, c0, 3      // Set the TLS.
    bl      __unlock_ready      // Unlock the ready list.
    pop     { SAVED_REGS }      // Restore registers
    rfeia   sp!                 // And dispatch.

/* Handle a system call.
 * On the ARM, this is entered with an SVC instruction.
 */
swi:
    srsfd   sp!, #Mode_SYS      // Save LR_svc and SPSR_svc on the sys stack.
    cpsid   i, #Mode_SYS        // Switch to sys mode.
    push    { SAVED_REGS }      // Save registers.
    cpsie   i                   // Enable IRQ
    mov     r8, sp              // Save the sp.
    push    { r0, r1 }
    bl      thread_self         // Get the current context.
    str     r8, [r0]            // Save sp for clone().
    pop     { r0, r1 }
    
    // In a system call.
    // r7 is the call number.
__in_syscall:                   // The stack frame is valid here.
    cmp     r7, #512
    ldr     r8, syscall_p       // Point to the syscall save area.
    str     r7, [r8]            // Save call number for error reporting.
    bhs     check_arm_syscall   // Jump if too big for a syscall.
    ldr     r8, swi_table_p     // Get the system call table.
    ldr     r8, [r8, r7, lsl #2]// Get the system call entry.
    ldr     r7, [sp, #4 * NUM_SAVED] // Get the SVC return address for clone().
    push    { r4, r5, r6, r7 }  // Put extra arguments on the stack.
    blx     r8                  // Dispatch. Return value is in r0/r1
    add     sp, #6 * 4          // Deallocate args and r0-r1 from SAVED_REGS.
    pop     { r2-r12, lr }      // SAVED_REGS - r0-r1
    rfeia   sp!                 // And return.

syscall_p:
    .word   syscall

/* ARM has a few "hidden" system calls.
 */
check_arm_syscall:
    ldr     r8, low
    cmp     r7, r8
    blo     unhandled_swi       // RICH: Incorrect
    ldr     r8, high
    cmp     r7, r8
    bhi     unhandled_swi
    and     r7, r7, #0x7
    adr     r8, ARM_SVC_Table   // Get the system call table.
    ldr     r7, [r8, r7, lsl #2]// Get the stystem call entry.
    blx     r7                  // Dispatch. Return value is in r0/r1
    pop     { SAVED_REGS }      // Restore registers.
    rfeia   sp!                 // And return.

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
    .word   set_tls
    .word   unhandled_swi
    .word   unhandled_swi

/* Set thread area.
 */
set_tls:
    push    { r1, lr }
    mcr     p15, 0, r0, c13, c0, 3
    mov     r1, r0
    bl      =thread_self
    str     r1, [r0, #4]        // Save the TLS in the current Thread.
    mov     r0, #0
    pop     { r1, lr }
    bx      lr

/* Get thread area.
 */
    .global __aeabi_read_tp
__aeabi_read_tp:
    .global __get_tls
    mrc     p15, 0, r0, c13, c0, 3
    bx      lr

// Unhandled system calls.
unhandled_swi:
    stmfd   sp!, {r12, lr}
    ldr     r12, swi_table_p    // Get the system call table.
    ldr     r12, [r12, #-4]     // Get the call set flag.
    cmp     r12, #0
    beq     1f                  // If not, too soon for printf.
    push    {r2-r5}             // Push extra arguments.
    mov     r3, r1
    mov     r2, r0
    ldr     r1, syscall_p       // And the system call number.
    ldr     r1, [r1]
    ldr     r0, string_p        // Get the format string.
    bl      printf
    add     sp, #16             // clean up the stack.
1:
    mov     r0, #-ENOSYS       
    ldmfd   sp!, {r12, pc}

string_p:
    .word   string

// Set a system call entry.
// r0 = the entry number.
// r1 - the handling function.
    .global     __set_syscall
__set_syscall:
    cmp     r0, #512
    ldrlo   r2, swi_table_p     // Get the system call table.
    strlo   r2, [r2, #-4]       // Set the called flag to non-zero.     
    strlo   r1, [r2, r0, lsl #2]  // Set the stystem call entry.
    movhs   r0, #-1
    movlo   r0, #0
    bx      lr

swi_table_p:
    .word   swi_table

    .data
    .global __heap_end
__heap_end:
    .word   0

syscall:
    .word   0

// Create the system call table with room for 512 entries.
    .word   0                   // != 0 if a system call has been defined.
swi_table:
    .rep    512
    .word   unhandled_swi
    .endr

string:
    .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
    .asciz "kernel"

