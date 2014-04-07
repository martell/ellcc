.global _Reset
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

Reset_Handler:
    ldr sp, =stack_top          // Set up stack for C function.
    mov fp, #0
    mov lr, #0
    str fp, [sp,#-4]!
    str lr, [sp,#-4]!

    // Copy the exception handlers to low memory.
    movw    r0, :lower16:_ResetEnd
    movw    r1, :lower16:_Reset
    movt    r0, :upper16:_ResetEnd
    movt    r1, :upper16:_Reset
    cmp     r1, r0
    beq     1f
    sub     r0, r0, #4
    sub     r0, r0, r1
    bfc     r0, #0, #2
    add     r2, r0, #4
    mov     r0, #0
    bl      __aeabi_memcpy
1:

    bl  _startup
    b   .

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

#define T_bit 0x20              // Thumb bit (5) of CPSR/SPSR.

SVC_Handler:
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

SysCall:
    .word   0

Other_SVC:
    b   .

OutOfRange:
    b   .

// Unhandled system calls.
Unhandled_SVC:
    stmfd   sp!, {r12, lr}
    mov     r6, r4                  // Shift arguments.
    mov     r5, r3
    mov     r4, r2
    mov     r3, r1
    mov     r2, r0
    ldr     r0, stringPtr           // Get the format string.
    ldr     r1, SysCall             // And the system call number.
    bl      printf

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
    .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d\n"

