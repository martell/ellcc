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

#define T_bit 0x20
#define F_bit 0x40
#define I_bit 0x80

.weak _init
.weak _fini

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

bss_start:
    .word __bss_start__
bss_end:
    .word __bss_end__

// main()'s pseudo arguments.
argv:
    .word   name
    .word   0                   // End of argv.
    .word   0                   // End of envp.
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv
    .word   0                   // Auxv

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
    srsfd   sp!, #Mode_SYS      // Save LR_svc and SPSR_svc on the sys mode stack.
    cpsid   i, #Mode_SYS        // Switch to sys mode.
    push    {r8, lr}            // Save registers.
    
    // In a system call.
    // r7 is the call number.
SVC_Frame:                      // The stack frame is valid here.
    cmp     r7, #512
    bhs     OutOfRange          // Jump if bad call.
    adr     r8, SVC_Table       // Get the system call table.
    str     r7, SysCall         // Save call number for error reporting.
    ldr     r7, [r8, r7, lsl #2]// Get the stystem call entry.
    blx     r7                  // Dispatch. Return value is in r0/r1
    pop     {r8, lr}            // Restore registers.
    rfeia   sp!                 // And return.

#if RICH
    stmfd   sp!, {r0-r3, r12, lr} // Store registers
    mov     r1, sp              // Set pointer to parameters
    mrs     r0, spsr            // Get spsr
    stmfd   sp!, {r0, r3}       // Store spsr onto stack and another
                                // register to maintain 8-byte-aligned stack
    tst     r0, #T_bit          // Occurred in Thumb state?
    ldrhne  r0, [lr,#-2]        // Yes: Load halfword and...
    bicne   r0, r0, #0xFF00     // ...extract comment field
    ldreq   r0, [lr,#-4]        // No: Load word and...
    biceq   r0, r0, #0xFF000000 // ...extract comment field
    // r0 now contains SVC number.
    // r1 now contains pointer to stacked registers.
    tst     r0, #0              // Is this a system call?
    bne     Other_SVC           // Jump if not
    // In a system call.
    // r7 is the call number, replace the call registers r0-r1.
    cmp     r7, #512
    bhs     OutOfRange          // Jump if bad call.
    adr     r0, SVC_Table       // Get the system call table.
    str     r7, SysCall         // Save call number for error reporting.
    ldr     r7, [r0, r7, lsl #2]// Get the stystem call entry.
    ldr     r0, [r1,#0]         // Restore argument registers.
    ldr     r1, [r1,#4]         //
    blx     r7                  // Dispatch.
    str     r0, [sp,#8]         // Save return values.
    str     r1, [sp,#12]
    ldmfd   sp!, {r2, r3}       // Get spsr from stack
    msr     SPSR_cxsf, r2       // Restore spsr
    ldmfd   sp!, {r0-r3, r12, pc}^ // Restore registers and return
#endif

SysCall:
    .word   0

Other_SVC:
    b   .

OutOfRange:
    b   .

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

    mvn     r0, #0                  // Return -1.
                                    // RICH: set errno
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
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC
    .word       Unhandled_SVC

    .data
string:
    .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
    .asciz "kernel"
