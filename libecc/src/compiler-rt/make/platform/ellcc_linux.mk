Description := Static runtime libraries for ellcc/Linux.

###

PWD := $(shell pwd)
ELLCC := $(PWD)/../../..
LIBECC := $(ELLCC)/libecc
override CC := $(ELLCC)/bin/ecc
RANLIB   := $(ELLCC)/bin/ecc-ranlib
Arch := unknown
Configs := $(patsubst %.notyet,,$(shell cd $(LIBECC)/mkscripts/targets; echo *))
INCLUDES := $(foreach TARGET, $(Configs), $(LIBECC)/mkscripts/targets/$(TARGET)/setup.mk)
OS := linux
CFLAGS := -Wall -Werror -O3 -fomit-frame-pointer
-include $(INCLUDES)

PROFILING_FUNCTIONS := GCDAProfiling InstrProfiling InstrProfilingBuffer \
                       InstrProfilingFile InstrProfilingPlatformOther \
                       InstrProfilingRuntime

###

#FUNCTIONS.arm := $(call set_difference, $(CommonFunctions), clear_cache) \

THUMB2_FUNCTIONS := \
	switch16 \
	switch32 \
	switch8 \
	switchu8 \
	sync_fetch_and_add_4 \
	sync_fetch_and_sub_4 \
	sync_fetch_and_and_4 \
	sync_fetch_and_or_4 \
	sync_fetch_and_xor_4 \
	sync_fetch_and_nand_4 \
	sync_fetch_and_max_4 \
	sync_fetch_and_umax_4 \
	sync_fetch_and_min_4 \
	sync_fetch_and_umin_4 \
	sync_fetch_and_add_8 \
	sync_fetch_and_sub_8 \
	sync_fetch_and_and_8 \
	sync_fetch_and_or_8 \
	sync_fetch_and_xor_8 \
	sync_fetch_and_nand_8 \
	sync_fetch_and_max_8 \
	sync_fetch_and_umax_8 \
	sync_fetch_and_min_8 \
	sync_fetch_and_umin_8

# ARM Assembly implementation which requires Thumb2 (i.e. won't work on v6M).
FUNCTIONS.arm := $(CommonFunctions) \
		 aeabi_idiv0 \
		 aeabi_idivmod \
		 aeabi_uidivmod \
		 aeabi_ldivmod \
		 aeabi_uldivmod \
		 aeabi_memset \
		 aeabi_memmove \
		 aeabi_memcpy \
		 aeabi_dcmp \
		 aeabi_fcmp \
		 $(THUMB2_FUNCTIONS) \
		 $(PROFILING_FUNCTIONS)

#FUNCTIONS.armeb := $(call set_difference, $(CommonFunctions), clear_cache) \

FUNCTIONS.armeb := $(CommonFunctions) \
		 aeabi_idiv0 \
		 aeabi_idivmod \
		 aeabi_uidivmod \
		 aeabi_ldivmod \
		 aeabi_uldivmod \
		 aeabi_memset \
		 aeabi_memmove \
		 aeabi_memcpy \
		 aeabi_dcmp \
		 aeabi_fcmp \
		 $(THUMB2_FUNCTIONS) \
		 $(PROFILING_FUNCTIONS)

FUNCTIONS.i386 := $(CommonFunctions) $(ArchFunctions.i386) \
		  $(PROFILING_FUNCTIONS)
FUNCTIONS.microblaze := $(CommonFunctions) $(ArchFunctions.microblaze)
FUNCTIONS.mips := $(CommonFunctions) $(ArchFunctions.mips) \
		  $(PROFILING_FUNCTIONS)
FUNCTIONS.mipsel := $(CommonFunctions) $(ArchFunctions.mipsel) \
		  $(PROFILING_FUNCTIONS)
FUNCTIONS.ppc := $(CommonFunctions) $(ArchFunctions.ppc) \
		  $(PROFILING_FUNCTIONS)
# RICH: FUNCTIONS.ppc64 := $(CommonFunctions) $(ArchFunctions.ppc64)
FUNCTIONS.x86_64 := $(CommonFunctions) $(ArchFunctions.x86_64) \
		  $(PROFILING_FUNCTIONS)

# Always use optimized variants.
OPTIMIZED := 1

# We don't need to use visibility hidden on Linux.
VISIBILITY_HIDDEN := 0
