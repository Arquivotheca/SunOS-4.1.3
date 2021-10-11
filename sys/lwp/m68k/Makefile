#
#	Copyright (c) 1989 Sun Microsystems, Inc.
#       @(#) Makefile 1.1 92/07/30
d-mc68020=mc68020
d-mc68010=mc68010
MACH= $(d$(TARGET_MACH))
LWPDIR= ..

INDHFILES= alloc.h cntxt.h common.h condvar.h \
	monitor.h schedule.h process.h queue.h \
	lwperror.h trace.h 

MACHHFILES= low.h lwpmachdep.h machdep.h param.h stackdep.h

HFILES= $(INDHFILES:%= $(LWPDIR)/%) $(MACHHFILES)

all: $(HFILES)

clean:

install: $(HFILES)

install_h: $(HFILES)
