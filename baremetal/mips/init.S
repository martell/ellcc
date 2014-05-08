    .section .text
    .align  4
    .global _start
_start:
    la      $k0, start
    jr      $k0
    nop

// The exception vector
    . = _start + 0x180
exception:
    .set    mips3
    mfc0    $k0, $14            // Get address of trapping instruction.
    addi    $k0, $k0, 4         // Point to the next instruction.
    mtc0    $k0, $14            // Put the new address back.
    eret                        // And return from the exception.
    .set    mips0
_start_end:

// Define the system stack size.
#define Stack_Size      4096

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

// Unhandled system calls.
unhandled_swi:
    b       .

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


