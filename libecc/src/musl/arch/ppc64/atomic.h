#ifndef _INTERNAL_ATOMIC_H
#define _INTERNAL_ATOMIC_H

#include <stdint.h>
#include <endian.h>

static inline int a_ctz_l(unsigned long x)
{
	static const char debruijn32[32] = {
		0, 1, 23, 2, 29, 24, 19, 3, 30, 27, 25, 11, 20, 8, 4, 13,
		31, 22, 28, 18, 26, 10, 7, 12, 21, 17, 9, 6, 16, 5, 15, 14
	};
	return debruijn32[(x&-x)*0x076be629 >> 27];
}

static inline int a_ctz_64(uint64_t x)
{
	uint32_t y = x;
	if (!y) {
		y = x>>32;
		return 32 + a_ctz_l(y);
	}
	return a_ctz_l(y);
}

static inline int a_cas(volatile int *p, int t, int s)
{

	__asm__( "1: lwarx 10, 0, %1\n"
                 "   stwcx. %3, 0, %1\n"
                 "   bne- 1b\n"
                 "   mr %0, 10\n"
		: "=r"(t) : "r"(p), "r"(t), "r"(s) : "memory" );
        return t;
}

static inline void *a_cas_p(volatile void *p, void *t, void *s)
{
	__asm__( "1: lwarx 10, 0, %1\n"
                 "   stwcx. %3, 0, %1\n"
                 "   bne- 1b\n"
                 "   mr %0, 10\n"
		: "=r"(t) : "r"(p), "r"(t), "r"(s) : "memory" );
	return t;
}

static inline long a_cas_l(volatile void *p, long t, long s)
{
	__asm__( "1: lwarx 10, 0, %1\n"
                 "   stwcx. %3, 0, %1\n"
                 "   bne- 1b\n"
                 "   mr %0, 10\n"
		: "=r"(t) : "r"(p), "r"(t), "r"(s) : "memory" );
	return t;
}


static inline int a_swap(volatile int *x, int v)
{
	int old;
	do old = *x;
	while (a_cas(x, old, v) != old);
	return old;
}

static inline int a_fetch_add(volatile int *x, int v)
{
	int old;
	do old = *x;
	while (a_cas(x, old, old+v) != old);
	return old;
}

static inline void a_inc(volatile int *x)
{
	a_fetch_add(x, 1);
}

static inline void a_dec(volatile int *x)
{
	a_fetch_add(x, -1);
}

static inline void a_store(volatile int *p, int x)
{
	*p=x;
}

static inline void a_spin()
{
}

static inline void a_crash()
{
	*(volatile char *)0=0;
}

static inline void a_and(volatile int *p, int v)
{
	int old;
	do old = *p;
	while (a_cas(p, old, old&v) != old);
}

static inline void a_or(volatile int *p, int v)
{
	int old;
	do old = *p;
	while (a_cas(p, old, old|v) != old);
}

static inline void a_and_64(volatile uint64_t *p, uint64_t v)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	a_and((int *)p, v);
	a_and((int *)p+1, v>>32);
#else
	a_and((int *)p+1, v);
	a_and((int *)p, v>>32);
#endif
}

static inline void a_or_64(volatile uint64_t *p, uint64_t v)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	a_or((int *)p, v);
	a_or((int *)p+1, v>>32);
#else
	a_or((int *)p+1, v);
	a_or((int *)p, v>>32);
#endif
}

#endif
