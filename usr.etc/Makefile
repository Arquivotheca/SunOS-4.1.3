#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.27 83/07/06
#
DESTDIR=
SYSDIR= ../sys
CFLAGS=	-O
LDFLAGS=
CC=	/bin/cc
CHOWN=/usr/etc/chown
MAKE= make CC=${CC}
LIBC=

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	admin automount c2convert cg8 config cron etherfind format gp gt \
	hostconfig htable in.ftpd in.named in.rwhod keyserv loadkeys mod \
	ndbootd openwin praudit rexd rfs rpc.bootparamd suninstall termcap \
	tfs yp zic

SUN2ONLY= sky
SUN3ONLY= eeprom fpa mc68881
SUN3XONLY= $(SUN3ONLY)
SUN4ONLY= eeprom fpuversion4
SUN4CONLY= $(SUN4ONLY)
ASP_4CN4M=unixname2bootname
SUN4MONLY= $(SUN4ONLY)

# Architecture specific programs
ASP= showrev 
ASDIR=/usr/kvm
RELASDIR=../kvm 
# Shell scripts that need only be installed and are never removed.
#
SCRIPT= extract_patch extract_unbundled sunlink.install C2conv \
	C2unconv audit_warn

SCRIPTSRC= $(SCRIPT:%=%.sh)

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD =	audit auditd catman chill chroot \
	dbconfig devnm edquota foption gettable grpck \
	in.comsat in.fingerd in.rexecd in.telnetd \
  	in.tnamed link mkfile mkproto \
	mount_hsfs mount_lo mount_pcfs mount_tmp \
	ping pwck quotaon repquota \
	route rpcinfo rusage \
	sa setsid swapon ttysoftcar tunefs tvconfig \
	unlink 

# C programs that live in the current directory and need explicit make lines.
#
NSTD= ac dcheck dumpfs exportfs icheck \
	ncheck newkey quot quotacheck rpc.mountd dkctl

# C programs that use the rpcsvc library (except for rpc.mountd)
#
RPCSVC=	rarpd rpc.etherd rpc.rquotad rpc.rstatd rpc.rwalld \
	rpc.pwdauthd rpc.rusersd rpc.showfhd rpc.sprayd rpc.yppasswdd \
	rpc.ypupdated rwall showfh showmount spray 

# C programs that use the libkvm library
#
KVM = savecore kgmon nfsstat trpt devinfo

# Programs that must run setuid to root
#
SETUID=	ping 

# Programs that must run setuid to audit
#
SETUIDAUDIT= auditd

# C programs that must run set-group-id kmem.
#
KMEM=	chill kgmon nfsstat trpt devinfo

# C programs that must run set-group-id operator.
#
OPERATOR=	dumpfs

# Programs built "small"
#
mkfile := LDFLAGS += -n -Bdynamic

all:	${SUBDIR} ${STD} ${NSTD} ${RPCSVC} ${KVM} machdep ${ASP}

machdep: FRC
	case `arch -k` in \
	sun2) \
		files="${SUN2ONLY}";; \
	sun3) \
		files="${SUN3ONLY}";; \
	sun3x) \
		files="${SUN3XONLY}";; \
	sun4) \
		files="${SUN4ONLY}";; \
	sun4c) \
		files="${SUN4CONLY} ${ASP_4CN4M}";; \
	sun4m) \
		files="${SUN4MONLY} ${ASP_4CN4M}";; \
	esac; \
	for i in $$files; do \
		if [ -d $$i ]; then \
			(cd $$i; ${MAKE} ${MFLAGS}); \
		else \
			make  $$i; \
		fi \
	done
	
${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${RPCSVC}:
	${CC} ${CFLAGS} -o $@ $@.c -lrpcsvc ${LIBC}

${KVM}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c -lkvm

install: ${STD} ${NSTD} ${RPCSVC} ${KVM} ${SCRIPTSRC} ${ASP}
	-install -d -o bin -m 755 ${DESTDIR}/usr/etc
	-install -d -o bin -m 755 ${DESTDIR}${ASDIR}
	@set -x; for i in ${SUBDIR}; do \
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); done
	case `arch -k` in \
	sun2) \
		files="${SUN2ONLY}";; \
	sun3) \
		files="${SUN3ONLY}";; \
	sun3x) \
		files="${SUN3XONLY}";; \
	sun4) \
		files="${SUN4ONLY}";; \
	sun4c) \
		for i in ${ASP_4CN4M}; do \
			(install -s $$i ${DESTDIR}${ASDIR}/$$i); \
		done; \
		files="${SUN4CONLY}";; \
	sun4m) \
		for i in ${ASP_4CN4M}; do \
			(install -s $$i ${DESTDIR}${ASDIR}/$$i); \
		done; \
		files="${SUN4MONLY}";; \
	esac; \
	for i in $$files; do \
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); \
	done
	@set -x; for i in ${SCRIPT}; do \
		(install -c $$i.sh ${DESTDIR}/usr/etc/$$i); done
	install -s ${STD} ${NSTD} ${RPCSVC} ${KVM} ${DESTDIR}/usr/etc
	rm -f ${DESTDIR}/usr/etc/quotaoff
	for i in ${SETUID}; do ($(CHOWN) root ${DESTDIR}/usr/etc/$$i; \
		chmod 4755 ${DESTDIR}/usr/etc/$$i); done
	for i in ${SETUIDAUDIT}; do ($(CHOWN) audit ${DESTDIR}/usr/etc/$$i; \
		chmod 4755 ${DESTDIR}/usr/etc/$$i); done
	-for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/usr/etc/$$i; \
		chmod 2755 ${DESTDIR}/usr/etc/$$i); done
	-for i in ${OPERATOR}; do (chgrp operator ${DESTDIR}/usr/etc/$$i; \
		chmod 2755 ${DESTDIR}/usr/etc/$$i); done
	ln ${DESTDIR}/usr/etc/quotaon ${DESTDIR}/usr/etc/quotaoff
	@set -x; for i in ${ASP}; do (install -s $$i ${DESTDIR}${ASDIR}/$$i; \
		rm -f ${DESTDIR}/usr/etc/$$i; \
		/usr/bin/ln -s ${RELASDIR}/$$i ${DESTDIR}/usr/etc/$$i); \
		done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC} ${KVM} ${ASP_4CN4M}

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

ac:	ac.c
	${CC} ${CFLAGS} -o ac ac.c

dcheck:	dcheck.c
	${CC} ${CFLAGS} -o dcheck dcheck.c

dumpfs: dumpfs.c
	${CC} -I${SYSDIR} ${CFLAGS} ${LDFLAGS} -o $@ dumpfs.c

exportfs: exportfs.o issubdir.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ exportfs.o issubdir.o

icheck:	icheck.c
	${CC} ${CFLAGS} -o icheck icheck.c

ncheck:	ncheck.c
	${CC} ${CFLAGS} -o ncheck ncheck.c

quot:	quot.c
	${CC} ${CFLAGS} -o quot quot.c

quotacheck:	quotacheck.c
	${CC} ${CFLAGS} -o quotacheck quotacheck.c

rpc.mountd: rpc.mountd.o issubdir.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ rpc.mountd.o issubdir.o -lrpcsvc

# this has a separate make line because of -lmp library.
newkey: newkey.o
	$(CC) ${CFLAGS} -o newkey newkey.o -lrpcsvc -lmp

# The following has an explicit make line because I want to make sure
# that it gets the latest <sun/dkio.h>

dkctl:	dkctl.c ${SYSDIR}/sun/dkio.h
	${CC} ${CFLAGS} -I${SYSDIR} -o dkctl dkctl.c

stripe:	stripe.c
	${CC} ${CFLAGS} -I${SYSDIR} -o stripe stripe.c

international:
	-rm -f newkey.o
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	$(MAKE) $(MFLAGS) "CFLAGS=$(CFLAGS) $(IFLAGS)" newkey)
	install -s newkey $(DESTDIR)/usr/etc
	-rm -f newkey
