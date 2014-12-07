/** Common definitions for crt1.S.
 */

#ifndef _crt1common_h_
#define _crt1common_h_

#include "config.h"
#include "context.h"

/** The following functions are defined  in crt1.S.
 */

#ifdef ELK_NAMESPACE
#define set_syscall __elk_set_syscall
#define switch_context __elk_switch_context
#define switch_context_arg __elk_switch_context_arg
#define enter_context  __elk_enter_context
#define new_context __elk_new_context
#define vector_copy __elk_vector_copy
#define cpu_init __elk_cpu_init
#define known_fault1 __elk_known_fault1
#define known_fault2 __elk_known_fault2
#define known_fault3 __elk_known_fault3
#define copy_fault __elk_copy_fault
#define cache_init __elk_cache_init
#define sploff __elk_sploff
#define splon __elk_splon
#endif

#define SYSCALL(name) set_syscall(SYS_ ## name, sys_ ## name)

/** Set a system call handler.
 * @param nr The system call number.
 * @param fn The system call handling function.
 * @return 0 on success, -1 on  error.
 * This function is defined in crt1.S.
 */
int set_syscall(int nr, void *fn);

/** Switch to a new context.
 * @param to The new context.
 * @param from A place to store the current context.
 * This function is implemented in crt1.S.
 */
int switch_context(context_t **to, context_t **from);

/** Switch to a new context.
 * @param arg The tenative return value when the context is restarted.
 * @param to The new context.
 * @param from A place to store the current context.
 * This function is implemented in crt1.S.
 */
int switch_context_arg(int arg, context_t **to, context_t **from);

/** Enter a new context.
 * @param to The new context.
 * This function is implemented in crt1.S.
 */
int enter_context(context_t **to);

/** Set up a new context.
 * @param savearea Where to put the finished stack pointer.
 * @param entry The context entry point (0 if return to caller).
 * @param mode The context execution mode.
 * @param arg1 The first argument to the entry point.
 * @param arg2 The second argument to the entry point.
 * @return 1 to indicate non-clone, else arg1.
 * This function is implemented in crt1.S.
 */
int new_context(context_t **savearea, void (entry)(void), int mode, long arg);

void vector_copy(vaddr_t);
void cpu_init(void);
void known_fault1(void);
void known_fault2(void);
void known_fault3(void);
void copy_fault(void);
void cache_init(void);
void sploff(void);
void splon(void);

/** The following functions are used by crt1.S.
 */
#ifdef ELK_NAMESPACE
#define enter_sys __elk_enter_sys
#define unlock_ready __elk_unlock_ready
#define leave_sys __elk_leave_sys
#define thread_self __elk_thread_self
#endif

/** Enter the IRQ state.
 */
void *enter_sys(void);

/** Unlock the ready queue.
 */
void unlock_ready(void);

/** Leave the IRQ state.
 */
void *leave_sys(void);

/** Get the current thread pointer.
 */
struct thread *thread_self();

#endif // _crt1common_h_

