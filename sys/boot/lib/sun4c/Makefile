#
# @(#)Makefile 1.1 92/07/30
#
# Makefile for Standalone Library

ARCH=		sun4c
ARCHDIR=	../sun4c
ARCHOBJ=	cache.o fiximp.o getlongprop.o getprop.o \
		intreg.o map.o mapin.o stubs.o
MODEL=		-DSUN4C_60

ARCHOPTS=	$(LIBPROMCPPOPTS)
ARCHINS=	$(LIBPROMINSTALL)
ARCHCLEAN=	$(LIBPROMCLEAN)

# Arch-specific device configuration as listed in $(ARCHDIR)/conf.c
CONFOBJ= boot_dk.o boot_if.o boot_romp.o boot_tp.o init_devsw.o

all: $$(LIBSA) $$(LIBPROM)

include ../sparc/Makefile.arch
include ../promlib/Makefile.prom
include ../common/Makefile.com
