#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.11 83/06/25
#
# make file for intermachine communications package
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				  mode 6??
#	${ADM}/aculog		ACU accounting file, owned by ${OWNER} and
#				  mode 6?? {if ACULOG defined}
# Presently supports:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	VENTEL 212+
#	VADIC 831 RS232 adaptor
#	VADIC 3451
#	HAYES SmartModem
# (drivers are located in aculib.a)
#
# Configuration defines:
#	DF02, DF03, DN11	ACU's supported
#	  BIZ1031, BIZ1022, VENTEL, V831, V3451, HAYES
#	ACULOG			turn on tip logging of ACU use
#	PRISTINE		no phone #'s put in ACU log file
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024

.DEFAULT:
	sccs get $@

ADM=	var/adm
OWNER=	uucp
GROUP=	daemon
CONFIG=	-DV831 -DVENTEL -DV3451 -DDF02 -DDF03 -DBIZ1031 -DBIZ1022 -DHAYES
CFLAGS=	-O -DDEFBR=300 -DDEFFS=BUFSIZ -DACULOG
LDFLAGS=
OBJS=	acu.o cmds.o cmdtab.o cu.o hunt.o \
	log.o partab.o remote.o tip.o tipout.o value.o vars.o
SPECIAL=acutab.c remcap.c uucplock.c

DRIVERS= aculib/biz22.c aculib/biz31.c aculib/df.c aculib/dn11.c \
	aculib/hayes.c aculib/v3451.c aculib/v831.c aculib/ventel.c

SOURCES=acu.c acutab.c cmds.c cmdtab.c cu.c hunt.c \
	log.c partab.c remote.c tip.c tipout.c value.c vars.c \
	${SPECIAL} ${DRIVERS}
CXREF=	/usr/ucb/ctags -xw

tip:	${OBJS} acutab.o remcap.o uucplock.o aculib/aculib.a
	${CC} -o tip ${LDFLAGS} ${OBJS} acutab.o remcap.o uucplock.o \
		aculib/aculib.a

${OBJS}: tip.h

remcap.o: remcap.c
	${CC} -c -DBUFSIZ=1024 ${CFLAGS} remcap.c

# acutab is configuration dependent, and so depends on the makefile
acutab.o: Makefile
acutab.o: acutab.c
	${CC} -c ${CFLAGS} ${CONFIG} acutab.c

# remote.o depends on the makefile because of DEFBR and DEFFS
remote.o: Makefile

# log.o depends on the makefile because of ACULOG
log.o:	Makefile

aculib/aculib.a: ${DRIVERS}
	cd aculib; make ${MFLAGS}

install: tip ${DESTDIR}/etc/remote ${DESTDIR}/${ADM}/aculog
	install -m 4711 -o ${OWNER} -g ${GROUP} tip ${DESTDIR}/usr/bin/tip
# we now use the System V version of cu
#	rm -f ${DESTDIR}/usr/bin/cu
#	ln ${DESTDIR}/usr/bin/tip ${DESTDIR}/usr/bin/cu

${DESTDIR}/etc/remote: etc.remote
	install -c -m 666 etc.remote ${DESTDIR}/etc/remote

${DESTDIR}/${ADM}/aculog:
	install -c -m 600 -o ${OWNER} /dev/null ${DESTDIR}/${ADM}/aculog

tags:	${SOURCES} tip.h
	ctags -w ${SOURCES} tip.h

clean:
	cd aculib; make ${MFLAGS} clean
	rm -f tip cu *.o

print:
	@pr README TODO makefile
	@size *.o | pr -h SIZES
	@${CXREF} tip.h ${SOURCES} | pr -h XREF
	@ls -l | pr 
	@pr tip.h ${SOURCES}

lint:
	-lint -hbacvx ${CFLAGS} ${SOURCES}
