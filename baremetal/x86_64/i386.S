
/** This is the reset start point.
 * Start in i386 mode. Switch to long mode.
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
    
// http://wiki.osdev.org/User:Stephanvanschaik/Setting_Up_Long_Mode

        .global _start
_start:
        // Disable paging, just in case.
        mov     %cr0, %eax              // Get control register 0.
        and     $0x7FFFFFFF, %eax       // Clear the PG bit.
        mov     %eax, %cr0              // To disable paging.

        // Set up four page tables at 0x1000.
        // First, clear the memory.
        pushl   $4096 * 4
        pushl   $0
        pushl   $0x1000
        call    memset
        add     $12, %esp

rich:
        // Set up cr3.
        mov     $0x1000, %edi
        mov     %edi, %cr3              // Set the control register.

        // Make PML4T[0] point to PDPT.
        movl    $0x2003, (%edi)         // Mark pages as present and R/W.
        add     $0x1000, %edi
        // Make PDPT point to PDT.
        movl    $0x3003, (%edi)
        add     $0x1000, %edi
        // Make PDT point to PT.
        movl    $0x4003, (%edi)
        add     $0x1000, %edi
        
        // Make PT point to 0x00000000 .. 0x00200000.
        mov     $0x00000003, %ebx       // Page present and writable.
        mov     $512, %ecx              // Number of entries.
1:
        mov     %ebx, (%edi)            // Save the page address.
        add     $4096, %ebx             // Page size.
        add     $8, %edi                // Next entry.
        loop    1b                      // Do all 512 entries.

        // Enable PAE paging.
        mov     %cr4, %eax              // Get the control register.
        or      $0x020, %eax            // Set the PAE bit.
        mov     %eax, %cr4

ready:
        // Switch to long mode.
        mov     $0xC0000080, %ecx       // Set the EFER MSR.
        rdmsr                           // Read the model specific register.
        or      $0x100, %eax            // Set the LM bit.
        wrmsr                           // Write the model specific register.

        // Enable paging.
        mov     %cr0, %eax
        or      $0x80000001, %eax       // Set the PG and protected mode bits.
        mov     %eax, %cr0

        // Enter the 64 bit submode.
        lgdt    [pointer]
        jmp     $code, $start

        jmp     .

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

