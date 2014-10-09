CFLAGS += -Werror -Wall -std=c99 -D_XOPEN_SOURCE=700
CFLAGS += -I$(SRCPATH)/$(LIB)/include/$(ARCH) \
          -I$(SRCPATH)/$(LIB)/include

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/$(ARCH)
# Startup code.
CRTSRCS += crt1.S
# Target specific code.
SRCS +=

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/kernel
# Target independent code.
SRCS += simple_console.c exit.c
