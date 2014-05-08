#include "target.h"

    .section .text
    .align  4
    .global _start
_start:
    la      $k0, start
    jr      $k0
    nop

/** The exception handler.
 * This has to be small. It is limited to 128 bytes.
 */
    . = _start + 0x180
    mfc0    $k1, $13            // Get the exception cause.
    andi    $k1, $k1, 0x7C      // Get the exception number.
    j       exception           // Handle the exception.
_start_end:

// Define the system stack size.
#define Stack_Size 4096

/** The reset entry point.
 */
start:
    move    $fp, $zero
    la      $sp, __mem_top
    // Set up the end of the heap.
    la      $t9, __mem_top - Stack_Size
    la      $t8, __heap_end
    sw      $t9, ($t8)
    la      $gp, _gp

    // Clear the bss area.
    la      $a0, __bss_start__
    move    $a1, $zero
    la      $a2, __bss_end__
    sub     $a2, $a2, $a0
    jal     memset
    nop

    // Copy the vectors.
    la      $a0, 0x80000000
    la      $a1, _start
    la      $a2, _start_end
    sub     $a2, $a2, $a1
    jal     memcpy
    nop
    
    and     $sp, $sp, -8                // Align the stack.
    addi    $sp, $sp , -4 * 4           // ABI requirement.
    addi    $a1, $zero, 1               // Get argc.
    la      $a2, argv                   // And argv.
    la      $a0, main                   // Get main's address.
    la      $t9, __libc_start_main
    jalr    $t9
    nop
    b       .
    nop

// main()'s pseudo arguments.
argv:
    .word   name
    .word   0                   // End of argv.
    .word   0                   // End of envp.
    .rept   32
    .word   0                   // Auxv
    .endr

/** Handle an exception.
 * k1 contains the exception number * 4.
 */
exception:
    addi    $sp, -CTX_SIZE
    .set    push
    .set    noat
    sw      $1, CTX_R1($sp)
    .set    pop
    sw      $2, CTX_R2($sp)
    sw      $3, CTX_R3($sp)
    sw      $4, CTX_R4($sp)
    sw      $5, CTX_R5($sp)
    sw      $6, CTX_R6($sp)
    sw      $7, CTX_R7($sp)
    sw      $8, CTX_R8($sp)
    sw      $9, CTX_R9($sp)
    sw      $10, CTX_R10($sp)
    sw      $11, CTX_R11($sp)
    sw      $12, CTX_R12($sp)
    sw      $13, CTX_R13($sp)
    sw      $14, CTX_R14($sp)
    sw      $15, CTX_R15($sp)
    sw      $16, CTX_R16($sp)
    sw      $17, CTX_R17($sp)
    sw      $18, CTX_R18($sp)
    sw      $19, CTX_R19($sp)
    sw      $20, CTX_R20($sp)
    sw      $21, CTX_R21($sp)
    sw      $22, CTX_R22($sp)
    sw      $23, CTX_R23($sp)
    sw      $24, CTX_R24($sp)
    sw      $25, CTX_R25($sp)
    sw      $gp, CTX_GP($sp)
    sw      $fp, CTX_FP($sp)
    sw      $ra, CTX_RA($sp)
    mfc0    $k0, $12            // Get the CP0 status register.
    sw      $k0, CTX_CP0_STATUS($sp)
    mfhi    $k0
    sw      $k0, CTX_HI($sp)
    mflo    $k0
    sw      $k0, CTX_LO($sp)
    mfc0    $k0, $14            // Get address of traping instruction.
    sw      $k0, CTX_PC($sp)
    lw      $k0, exception_table($k1)
    nop
    jalr    $k0                 /// Handle the exception.
    nop

    .set    push
    .set    noat
    lw      $1, CTX_R1($sp)
    .set    pop
    lw      $2, CTX_R2($sp)
    lw      $3, CTX_R3($sp)
    lw      $4, CTX_R4($sp)
    lw      $5, CTX_R5($sp)
    lw      $6, CTX_R6($sp)
    lw      $7, CTX_R7($sp)
    lw      $8, CTX_R8($sp)
    lw      $9, CTX_R9($sp)
    lw      $10, CTX_R10($sp)
    lw      $11, CTX_R11($sp)
    lw      $12, CTX_R12($sp)
    lw      $13, CTX_R13($sp)
    lw      $14, CTX_R14($sp)
    lw      $15, CTX_R15($sp)
    lw      $16, CTX_R16($sp)
    lw      $17, CTX_R17($sp)
    lw      $18, CTX_R18($sp)
    lw      $19, CTX_R19($sp)
    lw      $20, CTX_R20($sp)
    lw      $21, CTX_R21($sp)
    lw      $22, CTX_R22($sp)
    lw      $23, CTX_R23($sp)
    lw      $24, CTX_R24($sp)
    lw      $25, CTX_R25($sp)
    lw      $gp, CTX_GP($sp)
    lw      $fp, CTX_FP($sp)
    lw      $ra, CTX_RA($sp)
    lw      $k0, CTX_CP0_STATUS($sp)
    mtc0    $k0, $12            // Get the CP0 status register.
    lw      $k0, CTX_HI($sp)
    mthi    $k0
    lw      $k0, CTX_LO($sp)
    mtlo    $k0
    lw      $k0, CTX_PC($sp)
    mtc0    $k0, $14            // Set the return pc.
    .set    push
    .set    mips3
    eret                        // And return from the exception.
    .set    pop

// Unhandled exceptions.
unhandled_exception:
    lw      $k0, CTX_PC($sp)
    addi    $k0, $k0, 4         // Point to the next instruction.
    sw      $k0, CTX_PC($sp)
    jr      $ra

// Unhandled system calls.
unhandled_swi:
    b       .

/** Handle an interrupt exception.
 */
irq_exception:
    lw      $k0, CTX_PC($sp)
    addi    $k0, $k0, 4         // Point to the next instruction.
    sw      $k0, CTX_PC($sp)
    jr      $ra

/** Handle a system call exception.
 * v0 - the system call number.
 */
swi_exception:
    // RICH: Enable interrupts.
    move    $s0, $k0            // Save the system call.
    move    $s1, $sp            // Save the current sp.
    move    $s2, $ra            // Save the return address.
    // jal     thread_self         // Get the current context.
    // sw      $s1, ($v0)          // Save sp for clone().

__in_syscall:
    addi    $sp, $sp, -4 * 4    // Allocate stack space for four arguments.

    addi    $sp, $sp, 4 * 4     // Deallocate the aguments.
    lw      $k0, CTX_PC($sp)
    addi    $k0, $k0, 4         // Point to the next instruction.
    sw      $k0, CTX_PC($sp)
    jr      $s2

    .data
    .global __heap_end
__heap_end:
    .word   0

// Create the exception table with room for 32 entries.
exception_table:
    .word   irq_exception
    .rept   7
    .word   unhandled_exception
    .endr
    .word   swi_exception
    .rept    23
    .word   unhandled_exception
    .endr

// The current system call number.
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


