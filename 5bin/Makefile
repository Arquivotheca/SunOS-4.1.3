#
# @(#)Makefile 1.1 92/08/05 SMI
#
DESTDIR=
CFLAGS=	-O
CC=     /bin/cc
S5CC=	/usr/5bin/cc
MAKE=	make CC=${S5CC}

.KEEP_STATE:

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	diff3 m4 sed 

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	ar dircmp
SCRIPTSRC= ar.sh dircmp.sh

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	banner cat du echo expr nohup pr sum time tr df ln

# Programs that must run setuid to root
#
SETUID=	su

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	ls pg tabs 

# C programs that live in the current directory but
#   whose source is in /usr/src/usr.bin.
SHAREDUSRBIN=	chmod col date grep od sort stty touch su

all:	${SUBDIR} ${STD} ${NSTD} ${SHAREDUSRBIN} ${SETUID}

${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

${STD}:
	${S5CC} ${CFLAGS} -DS5EMUL -o $@ $@.c

${SHAREDUSRBIN}:
	(if [ ! -f ../usr.bin/$@.c ]; then cd ../usr.bin;sccs get $@.c; fi;)
	${CC} ${CFLAGS} -DS5EMUL -o $@ ../usr.bin/$@.c

install: ${STD} ${NSTD} ${SHAREDUSRBIN} ${SCRIPTSRC} ${SETUID}
	install -d -o bin -m 755 ${DESTDIR}/usr/5bin
	-for i in ${SUBDIR}; do \
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/5bin/$$i); done
	-for i in ${STD} ${NSTD} ${SHAREDUSRBIN}; do \
		(install -s $$i ${DESTDIR}/usr/5bin/$$i); \
	done
	install -s -m 4755 ${SETUID} ${DESTDIR}/usr/5bin
	rm -f ${DESTDIR}/usr/5bin/cc; ln -s ../5lib/compile ${DESTDIR}/usr/5bin/cc

clean:
	rm -f a.out y.tab.* core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${SHAREDUSRBIN}

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

ls:	ls.c
	${S5CC} -o ls ${CFLAGS} -DSINGLE -DS5EMUL ls.c -lcurses

pg:	pg.c
	${S5CC} -o pg ${CFLAGS} -DSINGLE -DS5EMUL pg.c -lcurses

tabs:	tabs.c
	${S5CC} -o tabs ${CFLAGS} -DS5EMUL tabs.c -lcurses

