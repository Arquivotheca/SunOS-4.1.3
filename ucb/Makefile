#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.12 85/09/17
#

DESTDIR=
INSDIR=$(DESTDIR)/usr/ucb

CFLAGS=	-O
# default is to build programs "small"
LDFLAGS = -n -Bdynamic

# where to find doprnt.c
DOPRNT= ../lib/libc/stdio/common

.KEEP_STATE:

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	Mail compress error ftp gprof \
	indent lpr more netstat rdist talk tftp \
	tset vgrind

# C Shell scripts that need only be installed and are never removed.
#
CSHSCRIPT= vtroff which
CSHSCRIPTSRC= $(CSHSCRIPT:%=%.csh)

# C programs that live in the current directory and have
# no special build requirements.
#
STD=	biff \
	checknr colcrt colrm ctags \
	expand \
	finger fold from \
	groups gcore \
	head \
	last lastcomm leave logger \
	man mkstr \
	printenv \
	rdate rlogin rsh ruptime rwho \
	sccs script soelim strings symorder \
	tail tcopy telnet \
	unexpand unifdef users \
	wc whereis whoami whois \
	xstr \
	yes

# C programs with special make rules. Each of these has its own target below.
SPECIAL= rcp

# C programs that use the rpcsvc library
RPCSVC=	quota rup rusers
$(RPCSVC) := LIBS += -lrpcsvc

# C programs that use the libkvm library
LIBKVM=	vmstat w
$(LIBKVM) := LIBS += -lkvm

# C programs that use the termlib library
TERMLIB= clear ul
$(TERMLIB) := LIBS += -ltermlib

# All programs in this directory
ALL= $(STD) $(SPECIAL) $(RPCSVC) $(LIBKVM) $(TERMLIB)

# All non-architecture specific programs
NONASP= $(STD) $(SPECIAL) $(RPCSVC) $(TERMLIB) vmstat

# All architecture specific programs
ASP= w
ASDIR= /usr/kvm
RELASDIR= ../kvm

# Larger and often-shared programs, linked demand paged
LARGE= finger man quota rlogin sccs telnet ul vmstat w
$(LARGE) := LDFLAGS=

# Programs that must run setuid to root
SETUID=	quota rcp rlogin rsh

# Programs that must run set-group-id kmem.
KMEM= vmstat w

all: $(ALL) $(SUBDIR)

$(SUBDIR): FRC
	cd $@ && $(MAKE) $(MFLAGS)

$(STD) $(RPCSVC) $(LIBKVM) $(TERMLIB): $$@.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(LIBS)

rcp:	$$@.c doprnt.o
	$(CC) $(CFLAGS) -Bstatic -o $@ $@.c doprnt.o

doprnt.o: $(DOPRNT)/doprnt.c
	$(COMPILE.c) -DFLOAT=0 -I$(DOPRNT) $(DOPRNT)/doprnt.c

# HACK: remove vmstat because it used to be a symlink
install: $(NONASP) $(ASP) $(CSHSCRIPTSRC)
	install -d -o bin -m 755 $(INSDIR)
	@set -x;for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) DESTDIR=$(DESTDIR) $@); done
	@set -x;for i in $(CSHSCRIPT); do \
		(install -c $$i.csh $(INSDIR)/$$i); done
	install -s $(NONASP) $(INSDIR)
	$(RM) $(INSDIR)/apropos $(INSDIR)/whatis
	ln $(INSDIR)/man  $(INSDIR)/apropos
	ln $(INSDIR)/man  $(INSDIR)/whatis
	install -s -o root -m 4755 $(SETUID) $(INSDIR)
	$(RM) $(INSDIR)/vmstat
	for i in $(ASP); do (install -s $$i $(DESTDIR)$(ASDIR)/$$i; \
		$(RM) $(INSDIR)/$$i; \
		ln -s $(RELASDIR)/$$i $(INSDIR)/$$i); \
		done
	$(RM) $(INSDIR)/vmstat
	install -g kmem -m 2755 vmstat $(INSDIR)
	install -g kmem -m 2755 w $(DESTDIR)$(ASDIR)
	$(RM) $(INSDIR)/uptime
	ln -s $(RELASDIR)/w $(INSDIR)/uptime
	rm -f ${DESTDIR}/usr/ucb/cc; ln -s ../lib/compile ${DESTDIR}/usr/ucb/cc

clean: FRC
	$(RM) a.out core *.s *.o
	$(RM) $(ALL)
	for i in $(SUBDIR); do (cd $$i && $(MAKE) $(MFLAGS) $@); done

FRC:
