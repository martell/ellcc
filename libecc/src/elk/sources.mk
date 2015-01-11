CFLAGS += -g -Werror -Wall -std=c11 -D_XOPEN_SOURCE=700 \
          -D_BSD_SOURCE -D_GNU_SOURCE
CFLAGS += -I$(SRCPATH)/$(LIB)/sys/$(ARCH) \
          -I$(SRCPATH)/$(LIB)/sys \
          -I$(SRCPATH)/$(LIB)/include

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/$(ARCH)
# Startup code.
CRTSRCS += crt1.S

# Target specific code.
SRCS.arm += arm_gic.c arm_sp804.c mmu.c cpufunc.S diag.c trap.c
SRCS.i386 += gdt.c idt.c i8259.c mmu.c cpufunc.S diag.c
SRCS.microblaze +=
SRCS.mips +=
SRCS.ppc += cpufunc.S diag.c
SRCS.x86_64 += idt.c diag.c

# Target independent code.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/kernel
# The ELK kernel.
SRCS += command.c time_commands.c \
	__elk_start.c time.c irq.c nirq.c thread.c file.c fdconsole.c \
	console.c memman.c simple_console.c simple_memman.c simple_exit.c \
	device.c network.c lwip_network.c unix_network.c

# LwIP
#define SIOCSIFNAME     0x8923
CFLAGS += -I$(SRCPATH)/$(LIB)/sys/lwip \
          -I$(SRCPATH)/$(LIB)/lwip/src/include \

# Turn off LwIP specific warnings.
CFLAGS += -Wno-empty-body -Wno-self-assign -Wno-unused-variable

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/core
# LwIP core functionality.
SRCS += def.c dhcp.c dns.c inet_chksum.c init.c mem.c memp.c netif.c pbuf.c \
        raw.c stats.c sys.c tcp.c tcp_in.c tcp_out.c timers.c udp.c
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/core/ipv4
SRCS += autoip.c icmp.c igmp.c ip4_addr.c ip4.c ip_frag.c
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/core/ipv6
SRCS += dhcp6.c ethip6.c icmp6.c inet6.c ip6_addr.c ip6.c ip6_frag.c mld6.c nd6.c
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/core/snmp
SRCS += asn1_dec.c asn1_enc.c mib2.c mib_structs.c msg_in.c msg_out.c
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/api
SRCS += api_lib.c api_msg.c err.c netbuf.c netdb.c netifapi.c pppapi.c tcpip.c
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/netif
SRCS += etharp.c ethernetif_driver.c lan91c111.c lan9118.c
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src/netif/ppp
SRCS += auth.c ccp.c chap-md5.c chap_ms.c chap-new.c demand.c eap.c ecp.c \
        eui64.c fsm.c ipcp.c ipv6cp.c lcp.c magic.c multilink.c ppp.c \
        pppcrypt.c pppoe.c pppol2tp.c upap.c utils.c vj.c
# ELK specific code.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/lwip/src
SRCS += sys_arch.c sio.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/mem
# Memory management.
SRCS += kmem.c page.c vm_nommu.c vm.c

# File systems.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/vfs
#Virtual file system.
SRCS += vfs_mount.c vfs_bio.c vfs_lookup.c vfs_security.c \
        vfs_vnode.c vfs_syscalls.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/devfs
# Device file system.
SRCS += devfs_vnops.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/ramfs
# RAM file system.
SRCS += ramfs_vnops.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/fifofs
# FIFO file system.
SRCS += fifo_vnops.c

#VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/binfs
# bin file system.
#SRCS += binfs_vnops.c

# Devices
# Serial devices.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/dev/serial
# ARM PL011 UART.
SRCS.arm += pl011.c
SRCS += serial.c tty.c cons.c

SRCS += $(SRCS.$(ARCH))
