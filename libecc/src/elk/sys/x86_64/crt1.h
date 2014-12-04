/** Definitions for crt1.S.
 */

#ifndef _crt1_h_
#define _crt1_h_

#include "config.h"

#ifdef ELK_NAMESPACE
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

void vector_copy(vaddr_t);
void cpu_init(void);
void known_fault1(void);
void known_fault2(void);
void known_fault3(void);
void copy_fault(void);
void cache_init(void);
void sploff(void);
void splon(void);

#endif // _crt1_h_
