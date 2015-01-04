/* Choose elk features to pull in. */

#ifdef ALL_COMMANDS
  #define TIME_COMMANDS
  #define PROC_COMMANDS
#endif

#ifdef SIMPLE_EXIT
  EXTERN(__elk_simple_exit) /* Simple infinate loop exit handling. */
#endif

#ifdef SIMPLE_CONSOLE
  EXTERN(__elk_simple_console) /* Simple polling console. */
#endif

#ifdef FD_CONSOLE
  #warning FD_CONSOLE is deprecated and will be removed.
  EXTERN(__elk_fdconsole) /* Temporary interrupt driven console */
  #define THREADS
#endif

#ifdef CONSOLE
  EXTERN(__elk_cons) /* The ELK console device. */
  #ifdef __arm__
    EXTERN(__elk_pl011_dev) /* ARM UART used as a console. */
  #endif
  #define THREADS
  #define RAMFS
  #define DEVFS
#endif

#ifdef THREADS
  EXTERN(__elk_thread) /* ELK thread handling. */
  #define TRAP
#endif

#ifdef TRAP
  EXTERN(__elk_trap) /* Handle traps. */
#endif

#ifdef TIME_COMMANDS
  EXTERN(__elk_time_commands) /* Time related commands. */
  #define COMMANDS
  #define TIME
#endif

#ifdef TIME
  EXTERN(__elk_time) /* Time related system calls. */
#endif

#ifdef SIMPLE_MEMMAN
  EXTERN(__elk_simple_memman) /* Simple malloc()/brk() memory management. */
#endif

#ifdef MMAP
  EXTERN(__elk_memman) /* Full mmap() and malloc() support. */
  #define VM
#endif

#ifdef VM
  #ifdef MMU
    EXTERN(__elk_vm) /* Full virtual memory support with MMU.  */
  #else
    EXTERN(__elk_vm_nommu) /* Non-MMU memory management. */
  #endif
#endif

#ifdef LWIP_NETWORK
  EXTERN(__elk_lwip_network) /* LWIP AF_INET networking. */
  #define NETWORK
  #ifdef LWIP_LAN91C111
    EXTERN(__elk_lwip_lan91c111) /* The LAN91C111 Ethernet driver.. */
  #endif
#endif

#ifdef UNIX_NETWORK
  EXTERN(__elk_unix_network) /* AF_UNIX (AF_LOCAL) networking. */
  #define NETWORK
#endif

#ifdef NETWORK
  EXTERN(__elk_network) /* Networking. */
  #define FS
#endif

#ifdef RAMFS
  EXTERN(__elk_ramfs) /* The RAM file system. */
  #define FS
#endif

#ifdef DEVFS
  EXTERN(__elk_devfs) /* The device file system. */
  #define FS
#endif

#ifdef FIFOFS
  EXTERN(__elk_fifofs) /* The fifo (pipe) file system. */
  #define FS
#endif

#ifdef FS
EXTERN(__elk_vfs_syscalls) /* File system syscalls. */
#endif

#ifdef TEST_COMMANDS
  EXTERN(__elk_test_commands) /* Test commands. */
  #define COMMANDS
#endif

#ifdef COMMANDS
  EXTERN(__elk_command) /* A simple command line interpreter. */
#endif


/* Miscellaneous commands. */
#ifdef PROC_COMMANDS
  EXTERN(__elk_mkdir_bin) /* The BSD mkdir program. */
  EXTERN(__elk_ls_bin)    /* The BSD ls program. */
  EXTERN(__elk_cat_bin)   /* The BSD cat program. */
  EXTERN(__elk_pwd_bin)   /* The BSD pwd program. */
  EXTERN(__elk_echo_bin)  /* The BSD echo program. */
  EXTERN(__elk_rm_bin)    /* The BSD rm program. */
#endif
