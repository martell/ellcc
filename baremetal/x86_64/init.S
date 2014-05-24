#include <bits/errno.h>                 // Get main's address.
#include "gdt.h"
#include "traps.h"
#include "entry.h"
#include "target.h"

/** Call a function only if it exists.
 * Act as if it returned 0 or NULL if it's not defined.
 */
#define weak_call(name)                         \
        .weak   name            ;               \
        movq    $name, %rax     ;               \
        cmp     $0, %rax        ;               \
        jz      1f              ;               \
        call    *%rax           ;               \
1:

        .data
        .align 16
GDT64:
        // The null descriptor.
        .word   0                       // Limit (low).
        .word   0                       // Base (low).
        .byte   0                       // Base (middle).
        .byte   0                       // Access.
        .byte   0                       // Granularity.
        .byte   0                       // Base (high).

        // The code descriptor.
code = . - GDT64
        .word   0                       // Limit (low).
        .word   0                       // Base (low).
        .byte   0                       // Base (middle).
        .byte   0x98                    // Access.
        .byte   0x20                    // Granularity.
        .byte   0                       // Base (high).

        // The data descriptor.
data = . - GDT64
        .word   0                       // Limit (low).
        .word   0                       // Base (low).
        .byte   0                       // Base (middle).
        .byte   0x90                    // Access.
        .byte   0x00                    // Granularity.
        .byte   0                       // Base (high).

pointer:
        .word   . - GDT64 - 1           // Limit.
        .long   GDT64                   // Low address.
        .long   0                       // High address.

// Define the system stack size.
#define Stack_Size 4096

/** This is the 64 bit start point.
 */
        .text
        .align  4
        .global _start

_start:
        lgdt    [pointer]
        cli
        mov     $data, %ax              // Set up the segment registers.
        mov     %ax, %ds
        mov     %ax, %es
        mov     %ax, %fs
        mov     %ax, %gs
        jmp     .                  // RICH: For now.
nowgo:

        cli                             // Disable interrupts.
        cld
        // movq    $__mem_top, %rsp        // Set up the stack.

        // Set up the end of the heap.
        movq    $__mem_top - Stack_Size, __heap_end

        // Clear the bss area.
        movq    $__bss_end__, %rdx
        sub     $__bss_start__, %rdx
        xor     %rsi, %rsi
        movq    $__bss_start__, %rdi
        call    memset
        add     $3 * 8, %rsp

        // Set up the GDT..
        // RICH: call    gdt_init                // Initialize the GDT.
        call    idt_init                // Initialize the IDT.

        // Set up the syscall instruction.
        mov     $0xC0000081, %rcx       // Set the STAR.
        // movq    $((code + 3) << 48) | (code << 32), %rax
        movq    $0x000B000800000000, %rax
        wrmsr
        mov     $0xC0000082, %rcx       // Set the LSTAR.
        mov     $syscall, %rax
        wrmsr
        mov     $0xC0000083, %rcx       // Set the CSTAR.
        mov     $syscall, %rax
        wrmsr
        mov     $0xC0000084, %rcx       // Set the SFMASK.
        // EF_TF|EF_DF|EF_IE|0x3000
#define EF_TF 0x00000100
#define EF_DF 0x00000400
#define EF_IE 0x00000200
        mov     $EF_TF|EF_DF|EF_IE|0x3000, %rax       // RICH: Check flags.
        wrmsr

        // Initialize the C library.
        movq    $argv, %rdx             // Get argv.
        movq    $1, %rsi                // And argc.
        movq    $main, %rdi             // Get main's address.
        call    __libc_start_main

done:   hlt
        jmp     done

// main()'s pseudo arguments.
argv:
        .quad   name
        .quad   0                       // End of argv.
        .quad   0                       // End of envp.
        // Auxv
        .quad   0
        
        .global gdt_flush
gdt_flush:
        lgdt (%rdi) 
        movw $(__KERNEL_DS), %ax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %fs
        movw %ax, %gs
        movw %ax, %ss
        ljmp  $(__KERNEL_CS), $flush
flush:
        ret

/** Set up the exception handlers.
 */
    EXCEPTION(0x00, t_excep0)  /* Divide By Zero Exception */
    EXCEPTION(0x01, t_excep1)  /* Debug Exception */
    EXCEPTION(0x02, t_excep2)  /* Non Maskable Interrupt Exception */
    EXCEPTION(0x03, t_excep3)  /* Breakpoint Exception */
    EXCEPTION(0x04, t_excep4)  /* Into Detected Overflow Exception */
    EXCEPTION(0x05, t_excep5)  /* Out of Bounds Exception */
    EXCEPTION(0x06, t_excep6)  /* Invalid Opcode Exception */
    EXCEPTION(0x07, t_excep7)  /* No Coprocessor Exception */
EXCEPTION_ERR(0x08, t_excep8)  /* Double Fault Exception */
    EXCEPTION(0x09, t_excep9)  /* Coprocessor Segment Overrun Exception */
EXCEPTION_ERR(0x0a, t_excep10) /* Bad TSS Exception */
EXCEPTION_ERR(0x0b, t_excep11) /* Segment Not Present Exception */
EXCEPTION_ERR(0x0c, t_excep12) /* Stack Fault Exception */
EXCEPTION_ERR(0x0d, t_excep13) /* General Protection Fault Exception */
EXCEPTION_ERR(0x0e, t_excep14) /* Page Fault Exception */
    EXCEPTION(0x0f, t_excep15) /* Unknown Interrupt Exception */
    EXCEPTION(0x10, t_excep16) /* Coprocessor Fault Exception */
EXCEPTION_ERR(0x11, t_excep17) /* Alignment Check Exception */
    EXCEPTION(0x12, t_excep18) /* Machine Check Exception */
    EXCEPTION(0x13, t_excep19) /* Reserved Exception */
    EXCEPTION(0x14, t_excep20) /* Reserved Exception */
    EXCEPTION(0x15, t_excep21) /* Reserved Exception */
    EXCEPTION(0x16, t_excep22) /* Reserved Exception */
    EXCEPTION(0x17, t_excep23) /* Reserved Exception */
    EXCEPTION(0x18, t_excep24) /* Reserved Exception */
    EXCEPTION(0x19, t_excep25) /* Reserved Exception */
    EXCEPTION(0x1a, t_excep26) /* Reserved Exception */
    EXCEPTION(0x1b, t_excep27) /* Reserved Exception */
    EXCEPTION(0x1c, t_excep28) /* Reserved Exception */
    EXCEPTION(0x1d, t_excep29) /* Reserved Exception */
    EXCEPTION(0x1e, t_excep30) /* Reserved Exception */
    EXCEPTION(0x1f, t_excep31) /* Reserved Exception */

excep_stub:
        SAVE_ALL

        mov     %rsp, %rax     /* Used for fastcall of handler */
// RICH:        call    exception_handler
        jmp     .

        RESTORE_ALL

/** Set up the interrupt handlers.
 */
IRQ_ENTRY(0) /* system timer (cannot be changed) */
IRQ_ENTRY(1) /* keyboard controller (cannot be changed) */
IRQ_ENTRY(2) /* cascaded signals from IRQs 8–15 */
IRQ_ENTRY(3) /* serial port controller for COM2(shared with COM4, if present) */
IRQ_ENTRY(4) /* serial port controller for COM1(shared with COM3, if present) */
IRQ_ENTRY(5) /* LPT port 2  or  sound card */
IRQ_ENTRY(6) /* floppy disk controller */
IRQ_ENTRY(7) /* LPT port 1 or sound card(8-bit Sound Blaster and compatibles) */
        
IRQ_ENTRY(8)  /* real-time clock */
IRQ_ENTRY(9)  /* open interrupt/available or SCSI host adapter */
IRQ_ENTRY(10) /* open interrupt / available  or  SCSI  or  NIC */
IRQ_ENTRY(11) /* open interrupt / available  or  SCSI  or  NIC */
IRQ_ENTRY(12) /* mouse on PS/2 connector */
IRQ_ENTRY(13) /* math co-processor/integrated FPU/inter-processor interrupt(use depends on OS) */
IRQ_ENTRY(14) /* primary ATA channel */
IRQ_ENTRY(15) /* secondary ATA channel */

irq_stub:
        SAVE_ALL

        mov     %rsp, %rax     /* Used for fastcall of handler */
// RICH:         call    irq_handler

        RESTORE_ALL

/** Set up the 32 bit syscall handler.
 */
        .global syscall_trap
syscall_trap:
        pushq   $(0)
        pushq   $(0x80)
        jmp     syscall_stub

syscall_stub:
        SAVE_ALL
        movq    %rax, syscall           // Save the system call number.
        cmp     $512, %rax
        jb      1f                      // Jump if too big.
        xor     %rax, %rax              // Illegal call number.
1:
        call    *swi_table(, %rax, 8)

        mov     %rax, (PT_EAX*8)(%rsp)  /* Save the return value */
        RESTORE_ALL

/** The 64 bit mode system call handler.
 */
syscall:
        SAVE_ALL
        movq    %rax, callnum           // Save the system call number.
        cmp     $512, %rax
        jb      1f                      // Jump if too big.
        xor     %rax, %rax              // Illegal call number.
1:
        call    *swi_table(, %rax, 8)
        RESTORE_ALL_64

// Unhandled system calls.
// The arguments are at 4($esp)
unhandled_swi:
        cmpq    $0, swi_table - 8       // Has a system call been defined?
        jnz     nosetup                 // Jump if so.
        weak_call(__setup_console)      // If not, set up the console.
nosetup:
        mov     %rsp, %r10              // Get the current stack pointer.
        pushq   %r9
        pushq   %r8
        mov     %rcx, %r9
        mov     %rdx, %r8
        mov     %rsi, %rcx
        mov     %rdi, %rdx
        movq    callnum, %rsi            // Send the system call number.
        movq    $swi_string, %rdi
        call    printf
        mov     $-ENOSYS, %rax
        ret

// Set a system call entry.
// %rdi = the entry number.
// %rsi - the handling function.
        .global     __set_syscall
__set_syscall:
        cmp     $511, %rdi
        ja      bad_call                // Jump if too big.
        mov     %rsi, swi_table(, %rax, 8)
        xor     %rax,%rax               // Return zero.
        ret

bad_call:
        mov     $-ENOSYS,%rax
        ret

#if RICH
    la      $k0, reset
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
reset:
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
#define AT_PAGESZ 6
argv:
    .word   name
    .word   0                   // End of argv.
    .word   0                   // End of envp.
    // Auxv
    .word   AT_PAGESZ
    .word   4096                // Page size.
    .word   0

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
    nop
    mfhi    $k0
    sw      $k0, CTX_HI($sp)
    nop
    mflo    $k0
    sw      $k0, CTX_LO($sp)
    nop
    mfc0    $k0, $14            // Get address of traping instruction.
    sw      $k0, CTX_PC($sp)
    nop
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
    nop
    mtc0    $k0, $12            // Get the CP0 status register.
    lw      $k0, CTX_HI($sp)
    nop
    mthi    $k0
    lw      $k0, CTX_LO($sp)
    nop
    mtlo    $k0
    lw      $k0, CTX_PC($sp)
    nop
    mtc0    $k0, $14            // Set the return pc.
    .set    push
    .set    mips3
    eret                        // And return from the exception.
    .set    pop

// Unhandled exceptions.
unhandled_exception:
    addi    $sp, $sp, -24
    sw      $ra, 20($sp)
    la      $a0, exception_string
    jal     puts
    nop
    lw      $ra, 20($sp)
    addi    $sp, $sp, 24
    jr      $ra
    nop

// Unhandled system calls.
// Four additional arguments are at ($sp)
unhandled_swi:

#define SAVE (4 * 12)
    addi    $sp, $sp, -SAVE
    sw      $ra, (4 * 11)($sp)
    sw      $s0, (4 * 10)($sp)
    lw      $s0, swi_table - 4  // Has a system call been set up?
    nop
    bnez    $s0, nosetup
    weak_call(__setup_console)  // If not, set up the console.
nosetup:
    // Get the additional arguments.
    lw      $s0, SAVE + (4 * 4)($sp)
    nop
    sw      $s0, (4 * 6)($sp)
    nop
    lw      $s0, SAVE + (4 * 5)($sp)
    nop
    sw      $s0, (4 * 7)($sp)
    nop
    lw      $s0, SAVE + (4 * 6)($sp)
    nop
    sw      $s0, (4 * 8)($sp)
    nop
    lw      $s0, SAVE + (4 * 7)($sp)
    nop
    sw      $s0, (4 * 9)($sp)

    sw      $a2, (4 * 4)($sp)
    sw      $a3, (4 * 5)($sp)
    move    $a3, $a1
    move    $a2, $a0
    lw      $a1, callnum
    la      $a0, swi_string
    jal     printf
    nop
    lw      $ra, (4 * 11)($sp)
    lw      $s0, (4 * 10)($sp)
    addi    $sp, $sp, SAVE

    addi    $v0, $zero, -ENOSYS
    jr      $ra
    nop

/** Handle an interrupt exception.
 */
irq_exception:
    lw      $k0, CTX_PC($sp)
    nop
    addi    $k0, $k0, 4         // Point to the next instruction.
    sw      $k0, CTX_PC($sp)
    // RICH: Handle it.
    jr      $ra
    nop

/** Handle a system call exception.
 * v0 - the system call number.
 */
swi_exception:
    lw      $k0, CTX_PC($sp)
    nop
    addi    $k0, $k0, 4         // Point to the next instruction.
    sw      $k0, CTX_PC($sp)

    // RICH: Enable interrupts.

    move    $s0, $v0            // Save the system call.
    move    $s1, $sp            // Save the current sp.
    move    $s2, $ra            // Save the return address.
    weak_call(thread_self)      // Get the current context.
    beqz    $v0, __in_syscall   // Jump if none.
    sw      $s1, ($v0)          // Save sp for clone().

__in_syscall:
    lw      $a0, CTX_R4($sp)    // Restore the system call arguments.
    lw      $a1, CTX_R5($sp)
    lw      $a2, CTX_R6($sp)
    lw      $a3, CTX_R7($sp)
    lw      $s4, CTX_SIZE + (4 * 4)($sp)
    lw      $s5, CTX_SIZE + (4 * 5)($sp)
    lw      $s7, CTX_SIZE + (4 * 6)($sp)
    lw      $s7, CTX_SIZE + (4 * 7)($sp)
    
    addi    $sp, $sp, -4 * 8    // Allocate stack space for four arguments.
    sw      $s4, (4 * 4)($sp)
    sw      $s5, (4 * 5)($sp)
    sw      $s6, (4 * 6)($sp)
    sw      $s7, (4 * 7)($sp)
    sw      $s0, syscall        // Save call number for error reporting.
    addi    $s4, $s0, -4000     // Subtract the offset.
    sltiu   $s5, $s4, 512       // Check for the maximum call number.
    beqz    $s5, invalid
    la      $s5, swi_table
    sll     $s4, $s4, 2         // Multiply by 4.
    add     $s5, $s5, $s4
    lw      $s5, ($s5)          // Get the function pointer.
    nop
    jalr    $s5                 // Call the handler.
    nop
back:
    addi    $sp, $sp, 4 * 8     // Deallocate the aguments.
    move    $s5, $zero          // No error flag.
    bgez    $v0, noerror        // Check for an error return
    slti    $s6, $v0, -4096     // Check for the maximum errno number.
    bnez    $s6, noerror
    addi    $s5, $zero, 1       // Error return (errno is -1 .. -4096).
    sub     $v0, $zero, $v0     // Return errno as a positive number.
noerror:
    sw      $s5, CTX_R7($sp)    // The error flag.
    sw      $v0, CTX_R2($sp)    // The syscall return values.
    sw      $v1, CTX_R3($sp)

    jr      $s2
    nop

invalid:
    jal     unhandled_swi
    nop
    b       back
    nop

/* Set up a context for execution.
 * int __new_context(Context **savearea,
 *                   int (*entry)(intptr_t, intptr_t), uint32_t psr,
 *                   void *ret, intptr_t arg1, intptr_t arg2)
 * On entry:
 * a0 points to the new context's save area, initialized to the top of the stack, aligned to 8 bytes.
 * a1 is the new context's start address
 * a2 is the new context's PSR.
 * a3 is the first argument to the context.
 * [sp] is the second argument to the context.
 */
#if RICH
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
#endif


/* Switch from one context to the next and unlock the queue.
 * void __switch(Context **to, Context **from);
 * On entry:
 * The ready list is locked.
 * a0 points to the next save area.
 * a1 points to the current save area.
 */
#if RICH
    .global __switch
__switch:
    srsfd   sp!, #Mode_SYS      // Save LR_sys and SPSR_sys on the sys stack.
    push    { SAVED_REGS }      // Store other registers.
    str     sp, [r1]            // Save the current frame.
    ldr     sp, [r0]            // Get the new stack pointer.
    ldr     r1, [r0, #4]        // Get the new TLS.
    mcr     p15, 0, r1, c13, c0, 3      // Set the TLS.
    weak_call(__unlock_ready)   // Clear the ready lock.
    pop     { SAVED_REGS }      // Restore registers
    rfeia   sp!                 // And dispatch.
#endif

/* Switch from one context to the next and unlock the queue.
 * void __switch_arg(int arg, Context **to, Context **from);
 * On entry:
 * The ready list is locked.
 * a0 is the tenative return value when the context is restarted.
 * a1 points to the next save area.
 * a2 points to the current save area.
 */
#if RICH
    .global __switch_arg
__switch_arg:
    srsfd   sp!, #Mode_SYS      // Save LR_sys and SPSR_sys on the sys stack.
    push    { SAVED_REGS }      // Store other registers.
    str     sp, [r2]            // Save the current frame.
    ldr     sp, [r1]            // Get the new stack pointer.
    ldr     r1, [r1, #4]        // Get the new TLS.
    mcr     p15, 0, r1, c13, c0, 3      // Set the TLS.
    weak_call(__unlock_ready)   // Clear the ready lock.
    pop     { SAVED_REGS }      // Restore registers
    rfeia   sp!                 // And dispatch.
#endif

// Set a system call entry.
// a0 = the entry number.
// a1 - the handling function.
    .global     __set_syscall
__set_syscall:
    addi    $sp, $sp, -8
    sw      $s0, 4($sp)
    addi    $a0, $a0, -4000     // Subtract the offset.
    sltiu   $s0, $a0, 513       // Check for the maximum call number.
    bnez    $s0, 1f
    addi    $v0, $zero, -1      // Return -1.
    lw      $s0, 4($sp)
    addi    $sp, $sp, 8
    jr      $ra
    nop

1:
    la      $s0, swi_table
    sll     $a0, $a0, 2         // Multiply by 4.
    sw      $s0, -4($s0)        // Set the called flag to non-zero.
    add     $s0, $s0, $a0
    sw      $a1, ($s0)          // Set the function pointer.
    lw      $s0, 4($sp)
    addi    $sp, $sp, 8
    jr      $ra
    nop

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
callnum:
    .word   0

// Create the system call table with room for 512 entries.
    .word   0                   // != 0 if a system call has been defined.
swi_table:
    .rep    512
    .word   unhandled_swi
    .endr

exception_string:
    .asciz "unhandled exception (%d)\n"
swi_string:
    .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
    .asciz "kernel"
#endif // RICH

        .data
        .global __heap_end
__heap_end:
        .quad   0

// The current system call number.
callnum:
        .quad   0

// Create the system call table with room for 512 entries.
        .quad   0               // != 0 if a system call has been defined.
swi_table:
        .rep    512
        .quad   unhandled_swi
        .endr

exception_string:
        .asciz "unhandled exception (%d)\n"
swi_string:
        .asciz "unhandled system call (%ld) args: %ld, %ld, %ld, %ld, %ld, %ld\n"
name:
        .asciz "kernel"
