#
#	Copyright (C) 1988 Sun Microsystems, Inc.
#       @(#) Makefile 1.1 92/07/30

SYSDIR= ..

INDHFILES= alloc.h cntxt.h common.h condvar.h \
	monitor.h schedule.h process.h queue.h \
	lwperror.h alloc.h trace.h cntxt.h 

MACHHFILES= low.h lwpmachdep.h machdep.h param.h stackdep.h

HFILES= $(INDHFILES:%= $(SYSDIR)/%) $(MACHHFILES)

all: $(HFILES)

clean:

install: $(HFILES)

install_h: $(HFILES)
