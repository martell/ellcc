#include <bits/errno.h>
#include "gdt.h"
#include "traps.h"
#include "entry.h"

/** Call a function only if it exists.
 * Act as if it returned 0 or NULL if it's not defined.
 */
#define weak_call(name)                         \
        .weak   name            ;               \
        movl    $name, %eax     ;               \
        cmp     $0, %eax        ;               \
        jz      1f              ;               \
        call    *%eax           ;               \
1:

/** This is the reset start point.
 */
        .section .text
        .align  4
// Multiboot header.
/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC      0x1BADB002

/* The magic number passed by a Multiboot-compliant boot loader. It's placed into EBX register by loader */
#define MULTIBOOT_BOOTLOADER_MAGIC  0x2BADB002

/* The flags for the Multiboot header. */
#define MULTIBOOT_MUSTKNOW          0x0000ffff
/* Align all boot modules on page (4KB) boundaries. */
#define MULTIBOOT_PAGE_ALIGN        0x00000001
/* Must be provided memory information in multiboot_info structure */
#define MULTIBOOT_MEMORY_INFO       0x00000002
/* Must be provided information about the video mode table */
#define MULTIBOOT_VIDEO_INFO        0x00000004
/* If bit 16 in the ‘flags’ word is set, then the fields at offsets 12-28 in the Multiboot header are valid.
 * his information does not need to be provided if the kernel image is in elf format,
 * but it must be provided if the images is in a.out format or in some other format
 */
#define MULTIBOOT_AOUT_KLUDGE       0x00010000


#define MULTIBOOT_HEADER_FLAGS     MULTIBOOT_MEMORY_INFO | MULTIBOOT_PAGE_ALIGN

        .long   MULTIBOOT_HEADER_MAGIC
        .long   MULTIBOOT_HEADER_FLAGS
        .long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
        .long   0
        .long   0
        .long   0
        .long   0
        .long   0
        .long   0
        .long   0
        .long   0
        .long   0
    

// Define the system stack size.
#define Stack_Size 4096

        .global _start
_start:
        cli                             // Disable interrupts.
        cld
        movl    $__mem_top, %esp        // Set up the stack.

        // Set up the end of the heap.
        movl    $__mem_top - Stack_Size, __heap_end

        // Clear the bss area.
        movl   $__bss_end__, %eax
        sub    $__bss_start__, %eax
        pushl   %eax
        pushl   $0
        pushl   $__bss_start__
        call    memset
        add     $3 * 4, %esp

        call    gdt_init                // Initialize the GDT.
        call    idt_init                // Initialize the IDT.

        // Initialize the C library.
        pushl   $argv                   // Get argv.
        pushl   $1                      // And argc.
        pushl   $main                   // Get main's address.
        call    __libc_start_main

done:   hlt
        jmp     done

// main()'s pseudo arguments.
argv:
        .long   name
        .long   0                       // End of argv.
        .long   0                       // End of envp.
        // Auxv
        .long   0
        
        .global gdt_flush
gdt_flush:
        movl 4(%esp), %eax
        lgdt (%eax) 
    
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

        mov     %esp, %eax     /* Used for fastcall of handler */
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

        mov     %esp, %eax     /* Used for fastcall of handler */
// RICH:         call    irq_handler

        RESTORE_ALL

/** Set up the syscall handler.
 */
        .global syscall_trap
syscall_trap:
        pushl   $(0)
        pushl   $(0x80)
        jmp     syscall_stub

syscall_stub:
        SAVE_ALL

        movl    %eax, syscall           // Save the system call number.
        call    *swi_table(, %eax, 4)

        mov     %eax, (PT_EAX*4)(%esp)  /* Save the return value */
        RESTORE_ALL

// Unhandled system calls.
// The arguments are at 4($esp)
unhandled_swi:
        cmpl    $0, swi_table - 4       // Has a system call been defined?
        jnz     nosetup                 // Jump if so.
        weak_call(__setup_console)      // If not, set up the console.
nosetup:
        mov     %esp, %eax              // Get the current stack pointer.
        pushl   24(%eax)
        pushl   20(%eax)
        pushl   16(%eax)
        pushl   12(%eax)
        pushl   8(%eax)
        pushl   4(%eax)
        pushl   syscall                 // Send the system call number.
        pushl   $swi_string
        call    printf
        add     $8 * 4, %esp
        mov     $-ENOSYS, %eax
        ret

// Set a system call entry.
// 4(%esp) = the entry number.
// 8(%esp) - the handling function.
        .global     __set_syscall
__set_syscall:
        mov     4(%esp), %eax           // Get the system call number.
        cmp     $512, %eax
        ja      bad_call                // Jump if too big.
        mov     8(%esp), %ebx           // Get the system call handler.
        mov     %ebx, swi_table(, %eax, 4)
        xor     %eax,%eax               // Return zero.
        ret

bad_call:
        mov     $-ENOSYS,%eax
        ret

        .data
        .global __heap_end
__heap_end:
        .long   0

// The current system call number.
syscall:
        .long   0

// Create the system call table with room for 512 entries.
        .long   0               // != 0 if a system call has been defined.
swi_table:
        .rep    512
        .long   unhandled_swi
        .endr

exception_string:
        .asciz "unhandled exception (%d)\n"
swi_string:
        .asciz "unhandled system call (%d) args: %d, %d, %d, %d, %d, %d\n"
name:
        .asciz "kernel"
