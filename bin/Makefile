#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.13 85/09/09
#
DESTDIR= /net/squaw/home/au
MACHINES= iAPX286 i386 m68k mc68010 mc68020 pdp11 sparc sun u370 u3b u3b2 \
	u3b5 u3b15 vax
ARCHITECTURE= sun2 sun3 sun4 sun386 sun3x sun4c sun4m
CFLAGS=	-O 
LDFLAGS=
CC=	/bin/cc
CHOWN=	/usr/etc/chown
DOPRNT=	../lib/libc/stdio/common
MAKE= make CC=${CC}
RM=	/usr/bin/rm -f
ROOT=   root
SPGRP=  daemon

.KEEP_STATE:

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	awk csh diff make sed sh

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	false true mach

SCRIPTSRC= $(SCRIPT:%=%.sh)

SCRIPTASP= arch

SCRIPTASPSRC= $(SCRIPTASP:%=%.sh)

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	chgrp cmp domainname dd env \
	hostid kill ldd line mkdir mt newgrp nice \
	pagesize pwd rm rmail rmdir sync \
	tee test who

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	ed login mail passwd 

# Architecture-specific programs
#
ASP= ps
ASP4MONLY= mpstat mps

# Install directory for architecture-specific programs, absolute path
#
ASDIR= /usr/kvm

# Install directory for architecture-specific programs, relative to /usr/bin
#
RELASDIR= ../kvm

# Programs that must run setuid to root
#
SETUID=	login mail newgrp passwd 

# Programs that must run set-group-id kmem
#
KMEM=

# Programs that must run set-group-id tty.
#
TTY=	wall write

# Programs that must run set-group-id operator.
#
OPERATOR=	

# Programs built "static" for disaster recovery and startup.
#
STATIC=	bar hostname mv tar

# Programs built "small"
#
chgrp cmp cp domainname dd \
env hostid hostname kill  \
line ldd mkdir mt mv newgrp nice pagesize passwd \
${ASP} ${ASP4MONLY} pwd rm rmail rmdir sync tee test who \
wall := LDFLAGS += -n -Bdynamic

all:	${SUBDIR} ${STD} ${NSTD} ${TTY} cp ${STATIC} ${ASP} ${ASP4MONLY}

${SUBDIR}: FRC
	cd $@; $(MAKE) ${MFLAGS}

${STD} ${TTY} cp:
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ $@.c

${STATIC}:	doprnt.o $$@.c
	${CC} ${LDFLAGS} -Bstatic ${CFLAGS} -o $@ $@.c doprnt.o

doprnt.o:	${DOPRNT}/doprnt.c
	${CC} ${CFLAGS} -c -DFLOAT=0 -I${DOPRNT} ${DOPRNT}/doprnt.c

install: ${STD} ${NSTD} ${TTY} cp ${STATIC} ${SCRIPTSRC} ${ASP} ${ASP4MONLY} \
         ${SCRIPTASPSRC}
	install -d -o bin -m 755 ${DESTDIR}/usr/bin
	install -d -o bin -m 755 ${DESTDIR}${ASDIR}
	install -s cp ${DESTDIR}/usr/bin/newcp; \
		mv ${DESTDIR}/usr/bin/newcp ${DESTDIR}/usr/bin/cp;
	$(RM) ${DESTDIR}/usr/bin/mv; cp mv ${DESTDIR}/usr/bin/mv; \
		strip ${DESTDIR}/usr/bin/mv; chmod 755 ${DESTDIR}/usr/bin/mv
	@set -x; for i in ${SUBDIR}; do \
		(cd $$i; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install); done
	@set -x; for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/bin/$$i); \
		done
	@set -x; for i in ${SCRIPTASP} ; \
		do (install -c $$i.sh ${DESTDIR}${ASDIR}/$$i; \
		rm -f ${DESTDIR}/usr/bin/$$i; \
		/usr/bin/ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/bin/$$i); done
	install -s ${STD} ${NSTD} ${TTY} bar hostname tar ${DESTDIR}/usr/bin
	@set -x; for i in ${ASP}; do (install -s $$i ${DESTDIR}${ASDIR}/$$i; \
		rm -f ${DESTDIR}/usr/bin/$$i; \
		/usr/bin/ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/bin/$$i); \
		done
	case `arch -k` in \
	sun4m) \
	set -x; for i in ${ASP4MONLY}; do (install -s $$i ${DESTDIR}${ASDIR}/$$i; \
		rm -f ${DESTDIR}/usr/bin/$$i; \
		/usr/bin/ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/bin/$$i); \
		done;; \
	esac;
	$(RM) ${DESTDIR}/usr/bin/cc; \
		/usr/bin/ln -s ../lib/compile ${DESTDIR}/usr/bin/cc
	@set -x; for i in ${SETUID}; do ($(CHOWN) root ${DESTDIR}/usr/bin/$$i; \
		chmod 4755 ${DESTDIR}/usr/bin/$$i); done
	@set -x; list="${KMEM}"; \
		for i in $$list; do (chgrp kmem ${DESTDIR}/usr/bin/$$i; \
		chmod 2755 ${DESTDIR}/usr/bin/$$i); done
	@set -x; for i in ${ASP}; do (chgrp kmem ${DESTDIR}${ASDIR}/$$i; \
		chmod 2755 ${DESTDIR}${ASDIR}/$$i); done
	case `arch -k` in \
	sun4m) \
	set -x; for i in ${ASP4MONLY}; do (chgrp kmem ${DESTDIR}${ASDIR}/$$i; \
		chmod 2755 ${DESTDIR}${ASDIR}/$$i); done;; \
	esac;
	@set -x; for i in ${TTY}; do (chgrp tty ${DESTDIR}/usr/bin/$$i; \
		chmod 2755 ${DESTDIR}/usr/bin/$$i); done
	$(RM) "${DESTDIR}/usr/bin/["; \
		/bin/ln ${DESTDIR}/usr/bin/test "${DESTDIR}/usr/bin/["
	$(RM) ${DESTDIR}/usr/bin/e; \
		/bin/ln ${DESTDIR}/usr/bin/ed ${DESTDIR}/usr/bin/e
	$(RM) ${DESTDIR}/usr/bin/red; \
		/bin/ln ${DESTDIR}/usr/bin/ed ${DESTDIR}/usr/bin/red
	@set -x; for i in ${MACHINES}; do \
		/bin/rm -f ${DESTDIR}/usr/bin/$$i; \
		/bin/ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/bin/$$i; \
		 done
	@set -x; for i in ${ARCHITECTURE}; do \
		/bin/rm -f ${DESTDIR}/usr/bin/$$i; \
		/bin/ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/bin/$$i; \
		done
	$(RM) ${DESTDIR}/usr/bin/chsh; \
		/usr/bin/ln ${DESTDIR}/usr/bin/passwd ${DESTDIR}/usr/bin/chsh
	$(RM) ${DESTDIR}/usr/bin/chfn; \
		/usr/bin/ln ${DESTDIR}/usr/bin/passwd ${DESTDIR}/usr/bin/chfn
	$(RM) ${DESTDIR}/usr/bin/yppasswd; \
		/usr/bin/ln ${DESTDIR}/usr/bin/passwd ${DESTDIR}/usr/bin/yppasswd
	$(RM) ${DESTDIR}/usr/bin/ypchsh; \
		/usr/bin/ln ${DESTDIR}/usr/bin/passwd ${DESTDIR}/usr/bin/ypchsh
	$(RM) ${DESTDIR}/usr/bin/ypchfn; \
		/usr/bin/ln ${DESTDIR}/usr/bin/passwd ${DESTDIR}/usr/bin/ypchfn

# create symlink from for ${DESTDIR}/bin for backward compatability.
#	/usr/bin/rm -rf ${DESTDIR}/bin
#	/usr/bin/ln -s /usr/bin ${DESTDIR}/bin

international:
	$(RM) ed passwd login
	$(RM) $(DESTDIR)/usr/bin/chfn
	$(RM) $(DESTDIR)/usr/bin/chsh
	$(RM) $(DESTDIR)/usr/bin/yppasswd
	$(RM) $(DESTDIR)/usr/bin/ypchfn
	$(RM) $(DESTDIR)/usr/bin/ypchsh
	${CC} ${CFLAGS} -o ed ed.c
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	 ${CC} ${CFLAGS} $(IFLAGS) -o login login.c -lrpcsvc)
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	 ${CC} ${CFLAGS} $(IFLAGS) -o passwd passwd.c -lrpcsvc)
	install -s ed ${DESTDIR}/usr/bin
	$(RM) ${DESTDIR}/usr/bin/e
	ln ${DESTDIR}/usr/bin/ed ${DESTDIR}/usr/bin/e
	$(RM) ${DESTDIR}/usr/bin/red
	ln ${DESTDIR}/usr/bin/ed ${DESTDIR}/usr/bin/red
	install -s -m 4755 passwd $(DESTDIR)/usr/bin
	install -s -m 4755 login $(DESTDIR)/usr/bin
	ln $(DESTDIR)/usr/bin/passwd $(DESTDIR)/usr/bin/chsh
	ln $(DESTDIR)/usr/bin/passwd $(DESTDIR)/usr/bin/chfn
	ln $(DESTDIR)/usr/bin/passwd $(DESTDIR)/usr/bin/yppasswd
	ln $(DESTDIR)/usr/bin/passwd $(DESTDIR)/usr/bin/ypchsh
	ln $(DESTDIR)/usr/bin/passwd $(DESTDIR)/usr/bin/ypchfn
	$(RM) ed passwd login

clean:
	$(RM) a.out core *.s *.o errs
	for i in ${SUBDIR}; do (cd $$i; $(MAKE) ${MFLAGS} clean); done
	$(RM) ${STD} ${NSTD} cp mv

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

ps:
	${CC} ${CFLAGS} $(LDFLAGS) -o ps ps.c -lm -lkvm

mps:
	case `arch -k` in \
	sun4m) \
	${CC} ${CFLAGS} $(LDFLAGS) -o mps -I/usr/src/sys mps.c -lm -lkvm ;;\
	esac; \

mpstat:
	case `arch -k` in \
	sun4m) \
	${CC} ${CFLAGS} $(LDFLAGS) -o mpstat -I/usr/src/sys mpstat.c -lkvm ;;\
	esac; \

ed:
	${CC} ${CFLAGS} -DCRYPT -o ed ed.c

rmjob:
	${CC} ${CFLAGS} ${LPINC} -c ${LPDIR}/rmjob.c

startdaemon:
	${CC} ${CFLAGS} ${LPINC} -c ${LPDIR}/startdaemon.c

common:
	${CC} ${CFLAGS} ${LPINC} -c ${LPDIR}/common.c

printcap:
	${CC} ${CFLAGS} ${LPINC} -c ${LPDIR}/printcap.c

displayq:
	${CC} ${CFLAGS} ${LPINC} -c ${LPDIR}/displayq.c

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

login:
	${CC} ${CFLAGS} -o login login.c -lrpcsvc

passwd:
	${CC} ${CFLAGS} -o passwd passwd.c -lrpcsvc

mail:
	${CC} ${CFLAGS} -o mail mail.c
