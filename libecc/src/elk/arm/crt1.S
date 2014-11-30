#include <bits/errno.h>
#include "config.h"
#include "asm.h"
#include "cpu.h"
#include "target.h"

#define KERNBASE 0      // RICH: This will change for high kernel.

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

	.section ".text","ax"
	.code 32
/** This is the reset start point.
 * For ARM this is a set of eight vectors, four bytes apart.
 * Use the vector to load the pc from a nearby handler function
 * address.
 */
        .global _start
_start:
	b	reset		        // Relative jump.
        .align  5
vector_start:
	/* Exception vector
	 *
	 * This table will be copied to an appropriate location.
	 * (the location is platform specific.)
	 */
	ldr	pc, reset_p	        // 0x00 mode: svc.
	ldr	pc, undefined_p	        // 0x04 mode: ?.
	ldr	pc, swi_p		// 0x08 mode: svc.
	ldr	pc, prefetch_p	        // 0x0c mode: abort.
	ldr	pc, abort_p	        // 0x10 mode: abort.
	nop				// 0x14 reserved.
	ldr	pc, irq_p		// 0x18 mode: irq.
	ldr	pc, fiq_p		// 0x1c mode: fiq.

reset_p:	.word	reset
undefined_p:	.word	undefined
swi_p:		.word	swi
prefetch_p:	.word	prefetch
abort_p:	.word	abort
irq_p:		.word	irq
fiq_p:		.word	fiq

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
        msr     cpsr_c, #Mode_FIQ|I_bit|F_bit
        ldr     sp, =__intstk_top__
        msr     cpsr_c, #Mode_IRQ|I_bit|F_bit
        ldr     sp, =__intstk_top__
        msr     cpsr_c, #Mode_ABT|I_bit|F_bit
        ldr     sp, =__abortstk_top__
        msr     cpsr_c, #Mode_UND|I_bit|F_bit
        ldr     sp, =__undefinedstk_top__
        msr     cpsr_c, #Mode_SVC|I_bit|F_bit
        ldr     sp, =__sysstk_top__
        // Finally, set up the SYS mode stack pointer and stay in SYS mode.
        msr     cpsr_c, #Mode_SYS       // |I_bit|F_bit
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
        str     r0, [r1]

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
	ldr	r1, =vector_start
	ldr	r2, =vector_end
1:
	ldmia	r1!, {r3}
	stmia	r0!, {r3}
	teq	r1, r2
	bne	1b
	mov	pc, lr

#ifdef CONFIG_CACHE
/*
 * Enable cache
 */
ENTRY(cache_init)
	mov	pc, lr
#endif

// main()'s pseudo arguments.
args:
  .word  1      // argc
argv:
        .word   name
        .word   0                       // End of argv.
        .word   0                       // End of envp.
        // Auxv
        .word   0

// RICH: TODO
undefined:
        ldr     r0, =undefined_string
        bl      diag_printf
        b       .

// RICH: TODO
prefetch:
        ldr     r0, =prefetch_string
        bl      diag_printf
        b       .

// RICH: TODO
abort:
        ldr     r0, =abort_string
        bl      diag_printf
        b       .

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
        and     r8, sp, #4              // Test stack alignment.
        sub     sp, sp, r8              // Adjust the stack if needed.
        push    { r8, r9 }              // Save the adjustment.
        ldr     r8, =swi_table          // Get the system call table.
        ldr     r8, [r8, r7, lsl #2]    // Get the system call entry.
        ldr     r7, [sp, #4 * (NUM_SAVED + 2)]// Get the SVC return address
                                        // for clone().
        push    { r4, r5, r6, r7 }      // Put extra arguments on the stack.
        blx     r8                      // Dispatch. Return value is in r0/r1
        add     sp, #4 * 4              // Deallocate args.
        pop     { r8, r9 }              // Restore the stack adjustment.
        add     sp, sp, r8              // And adjust.
        add     sp, #4 * 2              // Deallocate r0-r1.
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
        push    {r2-r5}                 // Push extra arguments.
        mov     r3, r1
        mov     r2, r0
        ldr     r1, =syscall            // And the system call number.
        ldr     r1, [r1]
        ldr     r0, =unhandled_string   // Get the format string.
        bl      diag_printf             // Print the unhandled system call.
        add     sp, #16                 // clean up the stack.
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
        ldr     r4, userlimit
        cmp     r0, r4
        bhi     ASM_LABEL(copy_fault)
        mov     r12, #0
        b       2f
1:
        ldrb    r3, [r12, r0]
ASM_LABEL(known_fault1):                // May be fault here.
        strb    r3, [r12, r1]
        add     r12, r12, #1
2:
        subs    r2, r2, #1
        bcs     1b
        mov     r0, #0      /* Set no error */
        ldmia   sp, {r4, r11, sp, pc}

userlimit:
        .word   __user_limit__

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
        ldr     r4, userlimit
        cmp     r1, r4
        bhi     ASM_LABEL(copy_fault)
        mov     r12, #0
        b       2f
1:
        ldrb    r3, [r12, r0]
ASM_LABEL(known_fault2):                // May be fault here.
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
        ldr     r4, userlimit
        cmp     r0, r4
        bhi     ASM_LABEL(copy_fault)
        mov     r12, #0
        b       2f
1:
        ldrb    r3, [r12, r0]
ASM_LABEL(known_fault3):                // May be fault here.
        strb    r3, [r12, r1]
        cmp     r3, #0
        beq     3f
        add     r12, r12, #1
2:
        subs    r2, r2, #1
        bcs     1b
        mov     r0, #(ENAMETOOLONG)
        b       4f
3:
        mov     r0, #0      /* Set no error */
4:
        ldmia   sp, {r4, r11, sp, pc}

/*
 * Fault entry for user access
 */
ENTRY(copy_fault)
        mov     r0, #(EFAULT)
        ldmia   sp, {r4, r11, sp, pc}

/*
 * int spl0(void);
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

/*
 * int splhigh(void);
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
