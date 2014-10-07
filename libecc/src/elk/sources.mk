CFLAGS += -Werror -Wall -std=c99 -D_XOPEN_SOURCE=700
CFLAGS += -I$(SRCPATH)/include/$(ARCH) -I$(SRCPATH)/include

VPATH := $(VPATH):$(SRCPATH)/elk/$(ARCH)
# Startup code.
CRTSRCS += crt1.S
# Target specific code.
SRCS +=

VPATH := $(VPATH):$(SRCPATH)/elk
# Target independent code.
SRCS +=
