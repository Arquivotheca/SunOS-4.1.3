#
#	Copyright (C) 1988 Sun Microsystems, Inc.
#       @(#) Makefile 1.1 92/07/30

SYSDIR= ../..

INDHFILES= agent.h asynch.h common.h condvar.h message.h \
	monitor.h schedule.h process.h queue.h clock.h \
	lwperror.h alloc.h trace.h cntxt.h 

MACHHFILES= except.h low.h lwpmachdep.h machdep.h machsig.h param.h stackdep.h

HFILES= $(INDHFILES:%= $(SYSDIR)/%) $(MACHHFILES)

all: $(HFILES)

clean:

install: $(HFILES)

install_h: $(HFILES)
