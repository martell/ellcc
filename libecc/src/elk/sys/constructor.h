/** Constructor definitions.
 */

#ifndef _constructor_h_
#define _constructor_h_

#ifndef NULL
#define NULL 0
#endif

#define is_used __attribute((__used__))

#undef weak_alias
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((weak, alias(#old)))

#undef strong_alias
#define strong_alias(old, new) \
    extern __typeof(old) new __attribute__((alias(#old)))

#define FEATURE(feature) \
char __elk_ ## feature = 0; \
strong_alias(__elk_ ## feature, __elk_feature_ ## feature);

#define FEATURE_CLASS(feature, function) \
char __elk_ ## feature = 0; \
strong_alias(__elk_ ## feature, __elk_feature_ ## function);

#define USE_FEATURE(feature) do \
{ \
 extern char __elk_ ## feature; \
 __elk_ ## feature = 1; \
} while (0)

#if !defined(__ELK__)
// Building ELK to run under Linux.
// We'll use C constructors for everything.
#define ELK_PRECONSTRUCTOR() \
static void __elk_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_init(void)

#define ELK_PRECONSTRUCTOR_BY_NAME(returns, name) \
returns name(void) \
    __attribute__((__constructor__, __used__)); \
returns name(void)

#define C_CONSTRUCTOR() \
static void __elk_c_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_c_init(void)

#define C_CONSTRUCTOR_BY_NAME(returns, name) \
returns name(void) \
    __attribute__((__constructor__, __used__)); \
returns name(void)

#else
// Building ELK to run bare bones.
// The ELK constructors come before and after C library initialization.
#define ELK_PRECONSTRUCTOR() \
static void __elk_preinit(void); \
static void (*__elk_preinit_p)(void) \
  __attribute((section (".elk_preinit_array"), __used__)) \
     = __elk_preinit; \
static void __elk_preinit(void)

#define ELK_SYSCONSTRUCTOR() \
static void __elk_sysinit(void); \
static void (*__elk_sysinit_p)(void) \
  __attribute((section (".elk_sysinit_array"), __used__)) \
     = __elk_sysinit; \
static void __elk_sysinit(void)

#define C_CONSTRUCTOR() \
static void __elk_c_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_c_init(void)

#define ELK_MAINCONSTRUCTOR() \
static void __elk_maininit(void); \
static void (*__elk_maininit_p)(void) \
  __attribute((section (".elk_maininit_array"), __used__)) \
     = __elk_maininit; \
static void __elk_maininit(void)

#endif // __ELK__

#endif // _constructor_h_
