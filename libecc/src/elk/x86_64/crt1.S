#include <bits/errno.h>                 // Get main's address.
#include "gdt.h"
#include "traps.h"
#include "entry.h"

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

        .word   0                       // Limit (low).
        .word   0                       // Base (low).
        .byte   0                       // Base (middle).
        .byte   0xF8                    // Access.
        .byte   0x20                    // Granularity.
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

        // Set up the GDT.
        // RICH: call    gdt_init                // Initialize the GDT.
        call    idt_init                // Initialize the IDT.

        // Set up the syscall instruction.
        mov     $0xC0000081, %rcx       // Set the STAR.
        // movq    $((code + 3) << 48) | (code << 32), %rax
        movq    $0x00000000, %rax
        movq    $0x00080008, %rdx
        wrmsr
        mov     $0xC0000082, %rcx       // Set the LSTAR.
        mov     $syscall, %rdx          // Get the syscal address.
        mov     %rdx, %rax              // Low part in %rax.
        shrq    $32, %rdx               // Hi part in %rdx.
        wrmsr
        mov     $0xC0000083, %rcx       // Set the CSTAR.
        wrmsr                           // Set the syscall address.
        mov     $0xC0000084, %rcx       // Set the SFMASK.
        // EF_TF|EF_DF|EF_IE|0x3000
#define EF_TF 0x00000100
#define EF_DF 0x00000400
#define EF_IE 0x00000200
        mov     $EF_TF|EF_DF|EF_IE|0x3000, %rax       // RICH: Check flags.
        wrmsr

        // Enable SSE. RICH: For now.
        mov     %cr0, %rax
        and     $0xFFFB, %ax            // Clear coprocessor emulation.
        or      $0x0002, %ax            // Set coprocessor monitoring.
        mov     %rax, %cr0
        mov     %cr4, %rax
        or      $3 << 9, %ax            // Set OSFXSR and OSXMMEXCPT
        mov     %rax, %cr4

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

        // mov     %rax, (PT_EAX*8)(%rsp)  /* Save the return value */
        RESTORE_ALL

/** The 64 bit mode system call handler.
 */
syscall:
	pushq   %rax                    // Save all the registers.
	pushq   %rbp
	pushq   %rdi
	pushq   %rsi
	pushq   %rdx
	pushq   %rcx
	pushq   %rbx
	pushq   %r8
	pushq   %r9
	pushq   %r10
	pushq   %r11
	pushq   %r12
	pushq   %r13
	pushq   %r14
	pushq   %r15

        movq    %rax, callnum           // Save the system call number.
        cmp     $512, %rax
        jb      1f                      // Jump if too big.
        xor     %rax, %rax              // Illegal call number.
1:
        call    *swi_table(, %rax, 8)

	pop     %r15
	pop     %r14
	pop     %r13
	pop     %r12
	pop     %r11
	pop     %r10
	pop     %r9
	pop     %r8
	pop     %rbx
	pop     %rcx
	pop     %rdx
	pop     %rsi
	pop     %rdi
	pop     %rbp
	pop     %rax
        mov     $0x0200, %r11           // Enable interrupts.
	sysretq

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
        mov     %rsi, swi_table(, %rdi, 8)
        xor     %rax,%rax               // Return zero.
        ret

bad_call:
        mov     $-ENOSYS,%rax
        ret


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
