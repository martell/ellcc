CFLAGS += -Werror -Wall -std=c99 -D_XOPEN_SOURCE=700

VPATH := $(VPATH):$(SRCPATH)/elk/$(ARCH)
CRTSRCS += crt1.S

