#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 11/22/82
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	libc csu

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; $(MAKE) ${MFLAGS}

install:
	install -d -o bin -m 755 ${DESTDIR}/usr/lib
	install -d -o bin -m 755 ${DESTDIR}/usr/5lib
	for i in ${SUBDIR}; do \
		(cd $$i; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install); done
	/bin/rm -f ${DESTDIR}/lib
	/bin/ln -s usr/lib ${DESTDIR}/lib
	/bin/rm -f ${DESTDIR}/usr/ucblib
	/bin/ln -s lib ${DESTDIR}/usr/ucblib

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; $(MAKE) ${MFLAGS} clean); done

FRC:
