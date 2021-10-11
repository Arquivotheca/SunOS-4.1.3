#
# @(#)Makefile 1.1 92/07/30 SMI
#

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)crash-3b2:crash.mk	1.11.1.1"

CFLAGS= -O
LDLIBS= -lkvm
LDFLAGS =
FRC =
# TARGET is the same as the host, by default, but may be overridden in the
# "make" command line for a cross-preprocessor.
TARGET	= `arch -k`

ASDIR = /usr/kvm
RELASDIR = ../kvm

OFILES= base.o \
	buf.o \
	callout.o \
	init.o \
	inode.o \
	main.o \
	map.o \
	mount.o \
	page.o \
	proc.o \
	rfs.o \
	search.o \
	size.o \
	sizenet.o \
	status.o \
	stream.o \
	symtab.o \
	u.o \
	util.o \
	vm.o \
	vtop.o

HFILES= crash.h \
	struct.h

CFILES= base.c \
	buf.c \
	callout.c \
	init.c \
	inode.c \
	main.c \
	map.c \
	mount.c \
	page.c \
	proc.c \
	rfs.c \
	search.c \
	size.c \
	sizenet.c \
	status.c \
	stream.c \
	symtab.c \
	u.c \
	util.c \
	vm.c \
	vtop.c

all: crash 

crash:	$(OFILES) 
	$(CC) $(CFLAGS) $(FFLAG) $(LDFLAGS) -o crash $(OFILES) $(LDLIBS)

$(OFILES): $(HFILES)

.c.o:
	@set -x; case ${TARGET} in \
	"sun2") \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Dmc68000 -Dmc68010 -Usparc -Dsun2 -c $*.c;;\
	"sun3") \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Dmc68000 -Dmc68020 -Usparc -Dsun3 -c $*.c;;\
        "sun3x") \
            $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
                -Dmc68000 -Dmc68020 -Usparc -Dsun3x -c $*.c;;\
	"sun4") \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Umc68000 -Umc68020 -Dsparc -Dsun4 -c $*.c;;\
	"sun4c") \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Umc68000 -Umc68020 -Dsparc -Dsun4c -c $*.c;;\
	"sun4m") \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Umc68000 -Umc68020 -Dsparc -Dsun4m -c $*.c;;\
	* ) \
	    echo 'Unknown machine???'; exit 2;;\
	esac

install: crash
	install -s -g kmem -m 2755 crash ${DESTDIR}${ASDIR}/crash
	rm -f ${DESTDIR}/usr/etc/crash ${DESTDIR}/etc/crash
	ln -s ${RELASDIR}/crash ${DESTDIR}/usr/etc/crash
	ln -s ../usr/etc/crash ${DESTDIR}/etc/crash

clean:
	rm -f $(OFILES) crash

lint: $(CFILES) $(HFILES) 
	lint $(COMFLAGS) -uh $(CFILES) 

FRC:
