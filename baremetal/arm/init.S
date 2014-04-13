#include <bits/errno.h>

.global _start
.global _Reset
_start:
_Reset:
    ldr pc, Reset_Addr
    ldr pc, Undefined_Addr
    ldr pc, SVC_Addr
    ldr pc, Prefetch_Addr
    ldr pc, Abort_Addr
    nop                         // Reserved. 
    ldr pc, IRQ_Addr
    ldr pc, FIQ_Addr
Reset_Addr:     .word Reset_Handler
Undefined_Addr: .word Undefined_Handler
SVC_Addr:       .word SVC_Handler
Prefetch_Addr:  .word Prefetch_Handler
Abort_Addr:     .word Abort_Handler
IRQ_Addr:       .word IRQ_Handler
FIQ_Addr:       .word FIQ_Handler
_ResetEnd:

#define Mode_USR 0x10
#define Mode_FIQ 0x11
#define Stack_FIQ 1024          // Size of the FIQ stack.
#define Mode_IRQ 0x12
#define Stack_IRQ 1024          // Size of the IRQ stack.
#define Mode_SVC 0x13
#define Stack_SVC 1024          // Size of the SVC stack.
#define Mode_ABT 0x17
#define Stack_ABT 1024          // Size of the ABT stack.
#define Mode_UND 0x1B
#define Stack_UND 1024          // Size of the UND stack.
#define Mode_SYS 0x1F
#define Stack_SYS 1024          // Size of the SYS stack.

#define T_bit 0x20
#define F_bit 0x40
#define I_bit 0x80

Reset_Handler:
    /* Set up the various stack pointers by setting the correct mode
     * and initializing the stack pointers.
     */
    msr cpsr_c, #Mode_FIQ|I_bit|F_bit
    ldr r0, =__mem_top           // Set up stack for FIQs.
    mov sp, r0
    sub r0, #Stack_FIQ
    msr cpsr_c, #Mode_IRQ|I_bit|F_bit
    mov sp, r0
    sub r0, #Stack_IRQ
    msr cpsr_c, #Mode_ABT|I_bit|F_bit
    mov sp, r0
    sub r0, #Stack_ABT
    msr cpsr_c, #Mode_UND|I_bit|F_bit
    mov sp, r0
    sub r0, #Stack_UND
    msr cpsr_c, #Mode_SVC|I_bit|F_bit
    mov sp, r0
    sub r0, #Stack_SVC
    // Finally, set up the SYS mode stack pointer and stay in SYS mode.
    msr cpsr_c, #Mode_SYS|I_bit|F_bit
    mov sp, r0
    sub r0, #Stack_SYS
    str r0, __heap_end

    // Copy the exception handlers to low memory.
    // RICH: Is this necessary? Change the vector address?
    mov a1, #0
    adr a2, _Reset
    adr a3, _ResetEnd
    sub a3, a3, a2
    bl  memcpy

    // Clear the bss area.
    ldr a1, bss_start
    mov a2, #0
    ldr a3, bss_end
    sub a3, a3, a1
    bl  memset

    // Now start up the system by initializing the library and calling main.
    mov fp, #0
    mov lr, #0
    str fp, [sp,#-4]!
    str a1, [sp,#-4]!
    adr a3, argv
    mov a2, #1
    ldr a1,=main
    bl  __libc_start_main
    b   .

    .global __heap_end
__heap_end:
    .word 0
bss_start:
    .word __bss_start__
bss_end:
    .word __bss_end__

// main()'s pseudo arguments.
argv:
    .word   name
    .word   0                   // End of argv.
    .word   0                   // End of envp.
    .rept   32
    .word   0                   // Auxv
    .endr

Undefined_Handler:
    b   .

Prefetch_Handler:
    b   .

Abort_Handler:
    b   .

IRQ_Handler:
    b   .

FIQ_Handler:
    b   .

SVC_Handler:
    srsfd   sp!, #Mode_SYS      // Save LR_svc and SPSR_svc on the sys stack.
    cpsid   i, #Mode_SYS        // Switch to sys mode.
    push    {r4-r12, lr}        // Save registers.
    
    // In a system call.
    // r7 is the call number.
__in_syscall:                   // The stack frame is valid here.
    cmp     r7, #512
    str     r7, SysCall         // Save call number for error reporting.
    bhs     check_arm_syscall   // Jump if too big for a syscall.
    adrlo   r8, SVC_Table       // Get the system call table.
    ldrlo   r7, [r8, r7, lsl #2]// Get the stystem call entry.
    blxlo   r7                  // Dispatch. Return value is in r0/r1
    pop     {r4-r12, lr}        // Restore registers.
    rfeia   sp!                 // And return.

/* ARM has a few "hidden" system calls.
 */
check_arm_syscall:
    ldr     r8, low
    cmp     r7, r8
    blo     Unhandled_SVC
    ldr     r8, high
    cmp     r7, r8
    bhi     Unhandled_SVC
    and     r7, r7, 0x7
    adr     r8, ARM_SVC_Table   // Get the system call table.
    ldr     r7, [r8, r7, lsl #2]// Get the stystem call entry.
    blx     r7                  // Dispatch. Return value is in r0/r1

    pop     {r4-r12, lr}        // Restore registers.
    rfeia   sp!                 // And return.

low:
    .word   0xF0000
high:
    .word   0xF0007

ARM_SVC_Table:
    .word   Unhandled_SVC
    .word   Unhandled_SVC
    .word   Unhandled_SVC
    .word   Unhandled_SVC
    .word   Unhandled_SVC
    .word   set_tls
    .word   Unhandled_SVC
    .word   Unhandled_SVC

/* Set thread area.
 */
set_tls:
    mcr     p15, 0, r7, c13, c0, 3
    mov     r0, 0
    bx      lr

/* Get thread area.
 */
    .global __aeabi_read_tp
__aeabi_read_tp:
    mrc     p15, 0, r0, c13, c0, 3
    bx      lr

SysCall:
    .word   0

// Unhandled system calls.
Unhandled_SVC:
    stmfd   sp!, {r12, lr}
    push    {r2-r5}                 // Push extra arguments.
    mov     r3, r1
    mov     r2, r0
    ldr     r1, SysCall             // And the system call number.
    ldr     r0, stringPtr           // Get the format string.
    bl      printf
    add     sp, #16                 // clean up the stack.

    mov     r0, #-ENOSYS       
    ldmfd   sp!, {r12, pc}

stringPtr:
    .word   string

// Set a system call entry.
// r0 = the entry number.
// r1 - the handling function.

    .global     __set_syscall
__set_syscall:
    adr     r2, SVC_Table       // Get the system call table.
    str     r1, [r2, r0, lsl #2]// Set the stystem call entry.
    bx      lr

// Create the system call table with room for 512 entries.
SVC_Table:
    .rep        512
    .word       Unhandled_SVC
    .endr

    .data
string:
    .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
    .asciz "kernel"

