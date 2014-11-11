#include <stdint.h>
#include <string.h>

int main();
void *__dso_handle = 0;
_Noreturn int __libc_start_main(int (*)(), int, char **);

#if defined(__ELK__)
extern char __data_start__[] __attribute__((weak));
extern char __data_end__[] __attribute__((weak));
extern char __text_end__[] __attribute__((weak));
extern char __bss_start__[] __attribute__((weak));
extern char __bss_end__[] __attribute__((weak));
extern void *__heap_end__;
extern void (*const __elk_init_array_end)() __attribute__((weak));
extern void (*const __elk_init_array_start)() __attribute__((weak));
extern void (*const __elk_init_array_end)() __attribute__((weak));

void __elk_start(long *p, void *heap_end)
{
  // Grab argc, and argv.
  int argc = p[0];
  char **argv = (void *)(p+1);

  // Copy initialized data from ROM.
  if ((uintptr_t)__data_start__ != (uintptr_t)__text_end__) {
    memcpy(__data_start__, __text_end__, __data_end__ - __data_start__);
  }

  // Clear the bss area.
  memset(__bss_start__, 0, __bss_end__ - __bss_start__);

  // Set up the end of the heap.
  // This has to be done after the data area is initialized.
  __heap_end__ = heap_end;

  // Run the ELK constructors.
  uintptr_t a = (uintptr_t)&__elk_init_array_start;
  for (; a<(uintptr_t)&__elk_init_array_end; a+=sizeof(void(*)()))
          (*(void (**)())a)();

  // Initialize the C run-time and call main().
  __libc_start_main(main, argc, argv);
}

#else
// For testing ELK under Linux.
#include <stdio.h>
#include "crt_arch.h"
void __cstart(long *p)
{
  int argc = p[0];
  char **argv = (void *)(p+1);
  __libc_start_main(main, argc, argv);
}

/** Set a system call handler.
 * @param nr The system call number.
 * @param fn The system call handling function.
 * @return 0 on success, -1 on  error.
 */
int __elk_set_syscall(int nr, void *fn)
{
  printf("__elk_set_syscall called: %d\n", nr);
  return 0;
}

#endif
