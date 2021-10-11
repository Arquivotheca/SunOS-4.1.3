#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.38 85/08/26
#

DESTDIR=
SYSDIR= ../sys
CFLAGS=	-O $(IFLAGS) -I$(SYSDIR)
DOPRNT= ../lib/libc/stdio/common
LDFLAGS=
CC= /bin/cc
S5CC= /usr/5bin/cc
CHOWN=	/usr/etc/chown

.KEEP_STATE:

# Programs that live in subdirectories, that should be built
# with the 4.2 C compiler, and that have makefiles of their own.
#
SUBDIR= calendar ctrace cu dc eqn file find graph \
	lex neqn nroff plot prof refer rpcgen screendump \
	spell sunpc sunwindows tbl tip trace troff uucp xsend \
	diff3 pax

# Programs that live in subdirectories, that should be built
# with the S5 C compiler, and that have makefiles of their own.
#
S5SUBDIR= ex nawk

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	diffmk dirname getoptcvt nohup desktop

SCRIPTSRC= $(SCRIPT:%=%.sh)

# C programs that live in the current directory, that should be built
# with the 4.2 C compiler, and that do not need explicit make lines.
#
STD=	basename cal cb checkeq click comm csplit cut \
	deroff eject fdformat getopt id installcmd \
	iostat ipcrm join logname look \
	mesg nl pack paste ptx \
	rev sdiff sleep split tsort tty \
	uniq units unpack uudecode uuencode uusend \
	xargs ypcat ypwhich ypmatch \
	cat df du echo pr sum time tr ls

# C programs that live in the current directory, that should be built
# with the S5 C compiler, and that do not need explicit make lines.
#
S5STD=	uname

# C programs that live in the current directory, that should be built
# with the 4.2 C compiler, and that use libkvm.a.
#
KVM=	ipcs

# YACC programs that live in the current directory, that should be built
# with the 4.2 C compiler, and that need explicit make lines.
#
YAC=	bc egrep

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	chkey cpio fgrep keylogin keylogout screenblank spline expr col \
	sort

# C programs that live in the current directory but whose source is
#  shared with ../5bin.
#
SHARED=	chmod date grep od stty su touch

# Programs that must run setuid to root
#
SETUID=	su

#Programs that must run set-group-id operator
#
OPERATOR= df

# C programs that must run set-group-id kmem.
#
KMEM=	iostat ipcs

# Programs built "small"
#
basename cal cb checkeq chmod click comm csplit crypt cut \
date deroff echo eject expr fdformat getopt id installcmd iostat \
ipcrm join ln logname look \
mesg nl pack paste ptx \
rev sdiff split stty tsort tty \
uniq units unpack xargs ypcat ypwhich ypmatch \
chkey cpio keylogin keylogout screenblank spline \
chmod col grep od sort \
cat df du sum time tr su ls := LDFLAGS += -n -Bdynamic

# Programs built "static" for disaster recovery and startup
#
STATIC=	ln

all:	${SUBDIR} ${S5SUBDIR} ${STD} ${S5STD} ${KVM} ${YAC} ${NSTD} gprof\
	${SHARED} ${STATIC}

${SUBDIR}: FRC
	cd $@; ${MAKE} CC=/bin/cc ${MFLAGS}

${S5SUBDIR}: FRC
	cd $@; ${MAKE} CC=/usr/5bin/cc ${MFLAGS}

${STD}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${S5STD}:
	${S5CC} ${CFLAGS} -DS5EMUL -o $@ $@.c

${SHARED}:
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

${STATIC}:	doprnt.o $$@.c
	${CC} ${LDFLAGS} -Bstatic -o $@ $@.c doprnt.o

doprnt.o:	$(DOPRNT)/doprnt.c
	${CC} ${CFLAGS} -c -DFLOAT=0 -I$(DOPRNT) $(DOPRNT)/doprnt.c

gprof:
	cd sunwindows; ${MAKE} CC=${CC} gprof

install: all ${SCRIPTSRC} ${SETUID} units.lib ptx.eign
	install -d -o bin -m 755 ${DESTDIR}/usr/bin
	@set -x;for i in ${SUBDIR}; do \
		(cd $$i; ${MAKE} CC=${CC} ${MFLAGS} DESTDIR=${DESTDIR} CHOWN=$(CHOWN) install); done
	@set -x; for i in ${S5SUBDIR}; do \
		(cd $$i; ${MAKE} CC=${S5CC} ${MFLAGS} DESTDIR=${DESTDIR} install); done
	@set -x; for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/bin/$$i); done
	install -s ${STD} ${S5STD} ${KVM} ${YAC} ${DESTDIR}/usr/bin
	install -s ${NSTD} ${SHARED} ${STATIC} ${DESTDIR}/usr/bin
	@set -x;for i in ${KMEM}; do (chgrp kmem ${DESTDIR}/usr/bin/$$i; \
		chmod 2755 ${DESTDIR}/usr/bin/$$i); done
	install -c -m 644 units.lib ${DESTDIR}/usr/lib/units
	install -c -m 644 ptx.eign ${DESTDIR}/usr/lib/eign
	rm -f ${DESTDIR}/usr/bin/cc; ln -s ../lib/compile ${DESTDIR}/usr/bin/cc
	rm -f ${DESTDIR}/usr/bin/pcat
	ln ${DESTDIR}/usr/bin/unpack ${DESTDIR}/usr/bin/pcat
	mv ${DESTDIR}/usr/bin/installcmd ${DESTDIR}/usr/bin/install
	for i in ${OPERATOR}; do (chgrp operator ${DESTDIR}/usr/bin/$$i; \
		chmod 2755 ${DESTDIR}/usr/bin/$$i); done
	install -s -m 4755 ${SETUID} ${DESTDIR}/usr/bin

international:
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	 ${CC} ${CFLAGS} -o chkey chkey.c -lrpcsvc -lmp)
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	 ${CC} ${CFLAGS} -o keylogin keylogin.c -lrpcsvc)
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	 ${CC} ${CFLAGS} -o keylogout keylogout.c -lrpcsvc)
	install -s chkey keylogin keylogout $(DESTDIR)/usr/bin
	$(RM) ${DESTDIR}/usr/bin/crypt
	$(RM) chkey keylogin keylogout

clean:
	rm -f a.out y.tab.* core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} CC=${CC} ${MFLAGS} clean); done
	for i in ${S5SUBDIR}; do (cd $$i; ${MAKE} CC=${S5CC} ${MFLAGS} clean); done
	rm -f ${STD} ${S5STD} ${KVM} ${YAC} ${NSTD} ${SHARED}

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

chkey: chkey.c
	${CC} ${CFLAGS} -o chkey chkey.c -lrpcsvc -lmp

cpio:	cpio.c
	${CC} -DVMUNIX ${CFLAGS} -o cpio cpio.c

keylogin: keylogin.c
	${CC} ${CFLAGS} -o keylogin keylogin.c -lrpcsvc

keylogout: keylogout.c
	${CC} ${CFLAGS} -o keylogout keylogout.c -lrpcsvc

screenblank:	screenblank.c
	${CC} -o screenblank ${CFLAGS} ${LDFLAGS} screenblank.c -lm

spline:	spline.c
	${CC} -o spline ${CFLAGS} spline.c -lm


$(YAC):
	${YACC} ${YFLAGS} $@.y
	${CC} -o $@ ${CFLAGS} -w y.tab.c
	rm -f y.tab.c

expr:
	${YACC} ${YFLAGS} $@.y
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} y.tab.c
	/usr/bin/rm -f y.tab.c

col:
	${CC} -o col ${CFLAGS} ${LDFLAGS} -DBSD col.c

sort:
	${CC} -o sort ${CFLAGS} ${LDFLAGS} -DPRE_S5R2 sort.c

# Following files have explicit make lines because they use libkvm.a

$(KVM):
	${CC} ${CFLAGS} -o $@ $@.c -lkvm

# Following files have explicit make lines because they
# are big and should be linked to be demand paged.

fgrep: fgrep.c
	${CC} ${CFLAGS} -o fgrep fgrep.c
