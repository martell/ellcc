/* Choose elk features to pull in. */

/*EXTERN(__elk_simple_console) /* Simple polling console. */
/*EXTERN(__elk_fdconsole) /* Temporary interrupt driven console */

EXTERN(__elk_cons) /* The ELK console device. */
EXTERN(__elk_pl011_dev) /* ARM UART used as a console. */

/*EXTERN(__elk_simple_memman) /* Simple malloc() memory management. */
EXTERN(__elk_memman) /* Full mmap() and malloc() support. */

EXTERN(__elk_thread) /* ELK thread handling. */

EXTERN(__elk_time) /* Time related system calls. */

/* EXTERN(__elk_vm) /* Full virtual memory support with MMU.  */
EXTERN(__elk_vm_nommu) /* Non-MMU memory management. */

EXTERN(__elk_vfs_syscalls) /* File system syscalls. */
EXTERN(__elk_ramfs) /* The RAM file system. */
EXTERN(__elk_devfs) /* The device file system. */
EXTERN(__elk_fifofs) /* The fifo (pipe) file system. */

EXTERN(__elk_command) /* A simple command line interpreter. */
EXTERN(__elk_time_commands) /* Time related commands. */
EXTERN(__elk_test_commands) /* Test commands. */

/* Miscellaneous commands. */
EXTERN(__elk_mkdir_bin) /* The BSD mkdir program. */
EXTERN(__elk_ls_bin)    /* The BSD ls program. */
EXTERN(__elk_cat_bin)   /* The BSD cat program. */
EXTERN(__elk_pwd_bin)   /* The BSD pwd program. */
EXTERN(__elk_echo_bin)  /* The BSD echo program. */
