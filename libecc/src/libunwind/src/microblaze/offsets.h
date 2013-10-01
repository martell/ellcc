/* Linux-specific definitions: */

/* Define various structure offsets to simplify cross-compilation.  */

/* Offsets for Microblaze Linux "ucontext_t":  */

#define LINUX_UC_FLAGS_OFF	0x0
#define LINUX_UC_LINK_OFF	0x4
#define LINUX_UC_STACK_OFF	0x8
#define LINUX_UC_MCONTEXT_OFF	0x14
#define LINUX_UC_SIGMASK_OFF	0xB0
#define LINUX_UC_MCONTEXT_REGS_OFF	0x14
