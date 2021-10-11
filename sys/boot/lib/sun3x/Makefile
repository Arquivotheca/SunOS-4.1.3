#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Makefile for Standalone Library

ARCH = sun3x
ARCHDIR= ../sun3x
ARCHOBJ= conf.o map.o atc.o getidprom.o
MODEL=  -DSUN3X_80 -DSUN3X_470
ARCHOPTS=

# Arch-specific device configuration as listed in $(ARCHDIR)/conf.c
CONFOBJ= gn_inf.o hllcputils.o hportllcp.o if_ec.o if_gn.o if_ie.o \
	if_le.o sc.o scsi.o sd.o se.o si.o sm.o st.o sw.o \
	tm.o xd.o xt.o xy.o

all: $$(LIBSA)

include ../m68k/Makefile.arch
include ../common/Makefile.com
