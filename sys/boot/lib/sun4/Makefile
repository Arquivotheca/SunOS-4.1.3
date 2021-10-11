#
# @(#)Makefile 1.1 92/07/30
#
# Makefile for Standalone Library

ARCH=	sun4
ARCHDIR=../sun4
ARCHOBJ=cache.o conf.o enable.o getidprom.o map.o stubs.o
MODEL=	-DSUN4_470 -DSUN4_330 -DSUN4_260 -DSUN4_110
ARCHOPTS="-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"

# Arch-specific device configuration as listed in $(ARCHDIR)/conf.c
CONFOBJ= gn_inf.o hllcputils.o hportllcp.o if_ec.o if_gn.o if_ie.o \
	if_le.o sc.o scsi.o sd.o se.o si.o sm.o st.o sw.o \
	tm.o xd.o xt.o xy.o ipi.o

all: $$(LIBSA)

include ../sparc/Makefile.arch
include ../common/Makefile.com
