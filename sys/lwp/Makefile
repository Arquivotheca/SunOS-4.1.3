#
#	Copyright (C) 1987 Sun Microsystems, Inc.
#       @(#) Makefile 1.1 92/07/30 Copyr 1987 Sun Micro
MAKE= /bin/make
MKDIR= /bin/mkdir
INSTALL= /usr/bin/install
RM= /bin/rm
CHMOD= /bin/chmod
CHOWN= /usr/etc/chown
d-mc68020=m68k
d-mc68010=m68k
d-sparc=sparc
MACH= $(d$(TARGET_MACH))
HFILES	=	chk.h cntxt.h common.h condvar.h lwp.h lwperror.h mch.h \
		monitor.h process.h queue.h schedule.h stk.h trace.h

kernel: $(HFILES) FRC
	cd $(MACH); $(MAKE)

clean:
	cd $(MACH); $(MAKE) clean

install install_h: $(HFILES)

FRC:
