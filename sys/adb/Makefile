#
# @(#)Makefile 1.1 92/07/30 SMI
#
# to add a new script add its name to SCRIPTS, check this Makefile back
# in under SCCS, then run make depend
#
# to check whether SCRIPTS is up to date run 'make check'
#
.KEEP_STATE:

.DEFAULT:
	sccs get -G$@ $(@D)/SCCS/s.$(@F)

DESTDIR=
SCRIPTS = aiodone aioresult as astoproc buf callout callout.nxt \
	calltrace calltrace.nxt cblock cblock.nxt clist condvar ctx cv \
	dblk dino dir dir.nxt direct direct.nxt dumphdr file \
	filsys findproc findproc.nxt forward forward.nxt fpu ifnet \
	inode inpcb iocblk iovec ipc ipreass ipreass.nxt kforward \
	kforward.nxt lwp lwpmach mact mact.nxt mblk mblk.nxt mbstat \
	mbuf mbuf.nxt mbufs mbufs.nxt memseg mntinfo mon monitor mount \
	msgbuf msgbuf.wrap nfsfattr page pcb pme pme.nxt pmenext \
	pmetopmg pmetov pmgrp proc protosw ptbl pty queue rawcb regs rnode \
	rtentry rusage seg segdev segmap segvn sess setproc \
	setproc.done setproc.nop setproc.nxt smap smap.find snode \
	sockbuf socket stack stacktrace stacktrace.nxt stat stdata \
	sysmap tcpcb tcpip tcpreass tcpreass.nxt thread tmount tmpnode \
	traceall traceall.nxt u u_fpu ucalltrace ucalltrace.nxt ucred uio \
	ustack vattr vnode vtimes wbuf.1buf wbuf

#These are scripts that are either obsolete or haven't been updated to
#4.1; in any event, they don't make.
NOTYET	= event msg

ASDIR = /usr/kvm
USRLIBDIR = /usr/lib

SRCS	= $(SCRIPTS:=.adb)

PROGS	= adbgen1 adbsub.o adbgen3 adbgen4

DESTDIR =
ARCH:sh	= arch -k
MACH:sh	= mach
FLAGS	= -w
RM	= rm -f
CC	= cc
CPPOPTS	= -I../${ARCH} -I.. -I../sys

.SUFFIXES: .adb

all:	${PROGS} ${SCRIPTS}

progs:	${PROGS}

# Note:
# adbgen.sh uses ${ARCH:-`arch -k`} which takes $ARCH, if present,
# from the environment, or substitutes `arch -k`.
# This means that, if make was initiated with 'ARCH=sunX' in the command line,
# adbgen.sh will make a sunX object.  Otherwise, it will compile for `arch -k`.
#
${SCRIPTS}: adbgen.sh $$(@F).adb ${PROGS}
	sh adbgen.sh ${FLAGS} $@.adb

adbgen1:	adbgen1.c
	$(CC) -O -o adbgen1 adbgen1.c

adbsub.o:	adbsub.c
	$(CC) -O -c adbsub.c

adbgen3:	adbgen3.c
	$(CC) -O -o adbgen3 adbgen3.c

adbgen4:	adbgen4.c
	$(CC) -O -o adbgen4 adbgen4.c

install: adbgen1 adbgen3 adbgen4 ${SCRIPTS} adbsub.o
	install -d -o bin -m 755 ${DESTDIR}${ASDIR}/adb
	-for i in adbgen1 adbgen3 adbgen4; do \
		install -s $$i ${DESTDIR}${ASDIR}/adb/$$i; done
	install -m 755 -c adbgen.sh ${DESTDIR}${ASDIR}/adb/adbgen
	install -g bin -m 644 ${SCRIPTS} adbsub.o ${DESTDIR}${ASDIR}/adb
	install -d -g bin -m 755 ${DESTDIR}${USRLIBDIR}
	-rm -rf ${DESTDIR}${USRLIBDIR}/adb
	ln -s ../kvm/adb ${DESTDIR}${USRLIBDIR}/adb

clean:
	${RM} ${SCRIPTS} adbgen adbgen[134] adbgen2* adbsub.o \
		script.files actual.files x.c

check:
	echo ${SCRIPTS} | tr ' ' '\012' | sed 's/$$/&.adb/' | sort > script.files
	ls *.adb > actual.files
	diff script.files actual.files
	${RM} script.files actual.files

maclist:
	@dir=`pwd`; for i in ${SCRIPTS}; do\
		echo $$dir/$$i; done
