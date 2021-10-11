#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=
CC=/bin/cc

SUBDIR= sys other

all: ${SUBDIR}

${SUBDIR}: FRC
	cd $@; ${MAKE} CC=${CC} ${MFLAGS}

clean: FRC
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done

install: FRC
	@set -x; for i in ${SUBDIR}; do \
		(cd $$i; ${MAKE} CC=${CC} ${MFLAGS} DESTDIR=${DESTDIR} install); done

FRC:
