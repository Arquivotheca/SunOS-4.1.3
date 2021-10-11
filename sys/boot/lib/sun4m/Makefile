#
# @(#)Makefile	1.1 7/30/92 SMI
#
# Makefile for Standalone Library

ARCH=		sun4m
ARCHDIR=	../sun4m
ARCHOBJ=	cache.o fiximp.o getlongprop.o getprop.o stubs.o
MODEL=		-DSUN4M_690

ARCHOPTS=	$(LIBPROMCPPOPTS)
ARCHINS=	$(LIBPROMINSTALL)
ARCHCLEAN=	$(LIBPROMCLEAN)

# Arch-specific device configuration as listed in $(ARCHDIR)/conf.c
# Printf doesn't really belong here, but...
CONFOBJ= boot_dk.o boot_if.o boot_romp.o boot_tp.o init_devsw.o

all: $$(LIBSA) $$(LIBPROM)

include ../sparc/Makefile.arch
include ../promlib/Makefile.prom
include ../common/Makefile.com
