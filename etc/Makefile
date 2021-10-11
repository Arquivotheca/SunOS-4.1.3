#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.6 85/08/26
#
DESTDIR=
CFLAGS=	-O
LDFLAGS=
DOPRNT= ../lib/libc/stdio/common

.KEEP_STATE:

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	dump files fsck getty ifconfig rpc.lockd rpc.statd in.routed restore

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	fastboot fasthalt

SCRIPTSRC= $(SCRIPT:%=%.sh)

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD_SHRD = biod chown clri fsirand inetd \
	newfs nfsd portmap reboot renice rmt \
	syslogd vipw ypbind arp dmesg dkinfo halt mknod

STD = init intr

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	update mkfs in.rlogind in.rshd mount

# C programs that use the rpcsvc library
#
RPCSVC=	shutdown umount

# C programs that must run set-group-id kmem.
#
KMEM=	arp dmesg

# Architecture-specific programs
#
ASP= pstat

# Architecture-specific programs in subdirectories
#
ASPSUBDIR= crash

# Architecture-specific programs that must run set-group-id kmem
#
ASKMEM= pstat

# Install directory for architecture-specific programs, absolute path
#
ASDIR= /usr/kvm

# Install directory for architecture-specific programs, relative path
#
RELASDIR= ../kvm

# C programs that must run set-group-id operator.
#
OPERATOR=	dkinfo

# Programs built "small"
#
chown fsirand dkinfo dmesg mkfs mknod newfs := LDFLAGS += -n -Bdynamic

# Programs which are installed in /usr/etc for which symlinks from /etc
# will be maintained.  NOTE: Some of these programs are built under usr.etc.
#
SYMLINKS= arp clri cron chown chroot config dkinfo dmesg dump fastboot \
	fasthalt fsck halt link mkfs mknod mount ncheck newfs \
	pstat rdump reboot renice restore rmt rrestore shutdown umount \
	update unlink vipw 

all:	${SUBDIR} ${STD} ${NSTD} ${RPCSVC} ${STD_SHRD} ${ASP} ${ASPSUBDIR}
 
${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

${ASPSUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS} "ASDIR=${ASDIR}" "RELASDIR=${RELASDIR}"

${STD_SHRD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${STD}:	doprnt.o $$@.c
	${CC} ${LDFLAGS} -Bstatic ${CFLAGS} -o $@ $@.c doprnt.o

doprnt.o:	${DOPRNT}/doprnt.c
	${CC} ${CFLAGS} -c -DFLOAT=0 -I${DOPRNT} ${DOPRNT}/doprnt.c

${RPCSVC}:
	${CC} ${CFLAGS} -o $@ $@.c ${LDFLAGS} -lrpcsvc

install: ${STD} ${NSTD} ${RPCSVC} ${STD_SHRD} ${SCRIPTSRC} ${ASP}
	install -d -o bin -m 755 ${DESTDIR}/etc
	install -d -o bin -m 755 ${DESTDIR}/var
	install -d -o bin -m 755 ${DESTDIR}/var/spool
	install -d -o bin -m 777 ${DESTDIR}/var/tmp
	install -d -o bin -m 755 ${DESTDIR}/var/preserve
	install -d -o bin -m 755 ${DESTDIR}/usr/etc
	install -d -o bin -m 755 ${DESTDIR}${ASDIR}
	install -d -o bin -m 755 ${DESTDIR}${ASDIR}/boot
	@set -x;for i in ${SUBDIR}; do \
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); done
	@set -x;for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/etc/$$i); done
	install -s ${STD} ${NSTD} ${RPCSVC} ${STD_SHRD} ${DESTDIR}/usr/etc
	@set -x;for i in ${ASP}; do \
		(install -s $$i ${DESTDIR}${ASDIR}/$$i; \
		rm -f ${DESTDIR}/usr/etc/$$i; \
		ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/etc/$$i); done
	@set -x;for i in ${ASPSUBDIR}; do \
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} ASDIR=${ASDIR} install); done
	@set -x;for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/usr/etc/$$i; \
		chmod 2755 ${DESTDIR}/usr/etc/$$i); done
	@set -x;for i in ${ASKMEM}; do (chgrp kmem ${DESTDIR}${ASDIR}/$$i; \
		chmod 2755 ${DESTDIR}${ASDIR}/$$i); done
	@set -x;for i in ${OPERATOR}; \
		do (chgrp operator ${DESTDIR}/usr/etc/$$i; \
		chmod 2755 ${DESTDIR}/usr/etc/$$i); done
	chown root.operator ${DESTDIR}/usr/etc/shutdown
	chmod 4754 ${DESTDIR}/usr/etc/shutdown
	install -d -o bin -m 755 ${DESTDIR}/usr/kvm/boot
	rm -rf ${DESTDIR}/usr/boot; ln -s ./kvm/boot ${DESTDIR}/usr/boot
	install -s mount ${DESTDIR}/usr/kvm/boot/mount
	install -s init ${DESTDIR}/usr/kvm/boot/init
	install -s intr ${DESTDIR}/usr/kvm/boot/intr
	install -s ifconfig/ifconfig ${DESTDIR}/usr/kvm/boot/ifconfig
	touch ${DESTDIR}/var/log/syslog
	chmod 664 ${DESTDIR}/var/log/syslog

#
# symlinks for compatibility with the old filesystem organization
#
	rm -rf ${DESTDIR}/usr/tmp
	ln -s ../var/tmp ${DESTDIR}/usr/tmp
	rm -rf ${DESTDIR}/etc/tmp
	ln -s ../var/tmp ${DESTDIR}/etc/tmp
	rm -rf ${DESTDIR}/usr/spool
	ln -s ../var/spool ${DESTDIR}/usr/spool
	rm -rf ${DESTDIR}/etc/spool
	ln -s ../var/spool ${DESTDIR}/etc/spool
	@set -x;for i in ${SYMLINKS}; do \
		$(RM) ${DESTDIR}/etc/$$i; \
		ln -s ../usr/etc/$$i ${DESTDIR}/etc/$$i; done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC} ${STD_SHRD}

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

pstat:	pstat.c
	${CC} ${CFLAGS} ${LDFLAGS}  -o pstat pstat.c -lkvm

update:	update.c
	${CC} ${CFLAGS} -o update -N update.c

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

mkfs:	mkfs.c
	${CC} ${LDFLAGS} ${CFLAGS} -o mkfs mkfs.c

in.rlogind:	in.rlogind.c
	${CC} ${CFLAGS} -o in.rlogind in.rlogind.c

in.rshd:	in.rshd.c
	${CC} ${CFLAGS} -o in.rshd in.rshd.c

mount:		mount.c
	${CC} ${CFLAGS} -Bstatic -o mount mount.c -lrpcsvc
