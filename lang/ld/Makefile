#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=

all: FRC
	cd ${CPU}; make ${MFLAGS} CC="${CC}" AS="${AS}"

xm68k: FRC
	cd m68k; make ${MFLAGS} CC="${CC}" AS="${AS}"

xsparc: FRC
	cd sparc; make ${MFLAGS} CC="${CC}" AS="${AS}"

xall: FRC
	cd m68k; make ${MFLAGS} CC="${CC}" AS="${AS}"
	cd sparc; make ${MFLAGS} CC="${CC}" AS="${AS}"

install: FRC
	cd ${CPU}; make ${MFLAGS} DESTDIR=${DESTDIR} install

clean:
	cd ${CPU}; make ${MFLAGS} clean

FRC:
