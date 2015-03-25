CXXFLAGS += -std=c99 -D_XOPEN_SOURCE=700
VPATH := $(VPATH):$(SRCPATH)/compiler-rt/lib/builtins

SRCS += absvdi2.c absvsi2.c absvti2.c adddf3.c addsf3.c addtf3.c addvdi3.c \
  addvsi3.c addvti3.c apple_versioning.c ashldi3.c ashlti3.c ashrdi3.c \
  ashrti3.c atomic.c clear_cache.c clzdi2.c clzsi2.c clzti2.c cmpdi2.c \
  cmpti2.c comparedf2.c comparesf2.c ctzdi2.c ctzsi2.c ctzti2.c divdc3.c \
  divdf3.c divdi3.c divmoddi4.c divmodsi4.c divsc3.c divsf3.c divsi3.c \
  divti3.c divxc3.c enable_execute_stack.c eprintf.c extendsfdf2.c \
  ffsdi2.c ffsti2.c fixdfdi.c fixdfsi.c fixdfti.c fixsfdi.c fixsfsi.c \
  fixsfti.c fixunsdfdi.c fixunsdfsi.c fixunsdfti.c fixunssfdi.c fixunssfsi.c \
  fixunssfti.c fixunsxfdi.c fixunsxfsi.c fixunsxfti.c fixxfdi.c fixxfti.c \
  floatdidf.c floatdisf.c floatdixf.c floatsidf.c floatsisf.c floattidf.c \
  floattisf.c floattixf.c floatundidf.c floatundisf.c floatundixf.c \
  floatunsidf.c floatunsisf.c floatuntidf.c floatuntisf.c floatuntixf.c \
  int_util.c lshrdi3.c lshrti3.c moddi3.c modsi3.c modti3.c muldc3.c muldf3.c \
  muldi3.c mulodi4.c mulosi4.c muloti4.c mulsc3.c mulsf3.c multi3.c multf3.c \
  mulvdi3.c mulvsi3.c mulvti3.c mulxc3.c negdf2.c negdi2.c negsf2.c negti2.c \
  negvdi2.c negvsi2.c negvti2.c paritydi2.c paritysi2.c parityti2.c \
  popcountdi2.c popcountsi2.c popcountti2.c powidf2.c powisf2.c powitf2.c \
  powixf2.c subdf3.c subsf3.c subvdi3.c subvsi3.c subvti3.c subtf3.c \
  trampoline_setup.c truncdfsf2.c ucmpdi2.c ucmpti2.c udivdi3.c udivmoddi4.c \
  udivmodsi4.c udivmodti4.c udivsi3.c udivti3.c umoddi3.c umodsi3.c umodti3.c \
  gcc_personality_v0.c mulsi3.c comparetf2.c fixunstfsi.c floatunsitf.c \
  extenddftf2.c fixtfsi.c floatsitf.c trunctfdf2.c trunctfsf2.c \
  extendsftf2.c divtf3.c

ifeq ($(ARCH),x86_64)
VPATH := $(VPATH):$(SRCPATH)/compiler-rt/lib/builtins/x86_64
SRCS += floatdidf.c floatdisf.c floatdixf.c \
  floatundidf.S floatundisf.S floatundixf.S
endif

ifeq ($(ARCH),i386)
VPATH := $(VPATH):$(SRCPATH)/compiler-rt/lib/builtins/i386
SRCS.i386 = ashldi3.S ashrdi3.S divdi3.S floatdidf.S \
  floatdisf.S floatdixf.S floatundidf.S floatundisf.S \
  floatundixf.S lshrdi3.S moddi3.S muldi3.S udivdi3.S \
  umoddi3.S
endif

ifeq ($(ARCH),arm)
VPATH := $(VPATH):$(SRCPATH)/compiler-rt/lib/builtins/arm
SRCS += aeabi_div0.c \
  aeabi_fcmp.S aeabi_idivmod.S aeabi_ldivmod.S aeabi_memcmp.S \
  aeabi_memcpy.S aeabi_memmove.S aeabi_memset.S \
  aeabi_uidivmod.S aeabi_uldivmod.S bswapdi2.S bswapsi2.S \
  comparesf2.S divmodsi4.S divsi3.S switch16.S \
  switch32.S switch8.S switchu8.S sync_fetch_and_add_4.S \
  sync_fetch_and_add_8.S sync_fetch_and_and_4.S \
  sync_fetch_and_and_8.S sync_fetch_and_max_4.S \
  sync_fetch_and_max_8.S sync_fetch_and_min_4.S \
  sync_fetch_and_min_8.S sync_fetch_and_nand_4.S \
  sync_fetch_and_nand_8.S sync_fetch_and_or_4.S \
  sync_fetch_and_or_8.S sync_fetch_and_sub_4.S \
  sync_fetch_and_sub_8.S sync_fetch_and_umax_4.S \
  sync_fetch_and_umax_8.S sync_fetch_and_umin_4.S \
  sync_fetch_and_umin_8.S sync_fetch_and_xor_4.S \
  sync_fetch_and_xor_8.S  sync_synchronize.S \
  udivmodsi4.S udivsi3.S umodsi3.S aeabi_dcmp.S
endif

ifeq ($(SUBARCH),armhf)
SRCS += adddf3vfp.S addsf3vfp.S \
  divdf3vfp.S divsf3vfp.S eqdf2vfp.S eqsf2vfp.S \
  extendsfdf2vfp.S fixdfsivfp.S fixsfsivfp.S fixunsdfsivfp.S \
  fixunssfsivfp.S floatsidfvfp.S floatsisfvfp.S \
  floatunssidfvfp.S floatunssisfvfp.S \
  gedf2vfp.S gesf2vfp.S gtdf2vfp.S \
  gtsf2vfp.S ledf2vfp.S lesf2vfp.S ltdf2vfp.S ltsf2vfp.S \
  modsi3.S muldf3vfp.S mulsf3vfp.S nedf2vfp.S negdf2vfp.S \
  negsf2vfp.S nesf2vfp.S restore_vfp_d8_d15_regs.S \
  save_vfp_d8_d15_regs.S subdf3vfp.S subsf3vfp.S \
  unorddf2vfp.S unordsf2vfp.S truncdfsf2vfp.S
endif

