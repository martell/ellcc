/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

/** Set a system call handler.
 * @param nr The system call number.
 * @param fn The system call handling function.
 * @return 0 on success, -1 on  error.
 */
int __set_syscall(int nr, void *fn);

/** Set up a new context.
 * @param savearea Where to put the finished stack pointer.
 * @param entry The context entry point.
 * @param mode The context execution mode.
 * @param ret The context return address.
 * @param arg1 The first argument ro the entry point.
 * @param arg2 The second argument to the entry point.
 */
void __new_context(void **savearea, int (*entry)(intptr_t, intptr_t), int mode, void *ret, intptr_t arg1, intptr_t arg2);

/** Switch to a new context.
 * @param from A place to store the current context.
 * @param to The new context.
 */
void __switch(void **from, void *to);

#endif // _kernel_h_
