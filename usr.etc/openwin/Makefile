#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1992 by SMCC
#

# Install directory for architecture-specific programs, absolute path
#
DESTDIR=
ARCH:sh = arch -k
SYSDIR=/usr/kvm/sys
OPENWIN_MODULES=/etc/openwin/modules 

OBJS= evqload evqmod-sun4.o evqmod-sun4c.o evqmod-sun4m.o \
	rpc.cmsd rpc.ttdbserverd winlock-sun4.o winlock-sun4c.o \
	winlock-sun4m.o winlockload

DAEMON=rpc.cmsd rpc.ttdbserverd
MODULES=evqload winlockload

all: ${OBJS}

install: ${OBJS}
	-install -d -o root -m 755 ${DESTDIR}${OPENWIN_MODULES}
	@set -x; for i in ${MODULES}; do \
		(rm -f ${DESTDIR}${OPENWIN_MODULES}/$$i; \
		install -m 755 $$i ${DESTDIR}${OPENWIN_MODULES}/$$i); \
	done
	-install -d -o bin -m 755 ${DESTDIR}/usr/etc
	@set -x; for i in ${DAEMON}; do \
		(rm -f ${DESTDIR}/usr/etc/$$i; \
		install -m 755 $$i ${DESTDIR}/usr/etc/$$i); \
	done 
	-install -d -o bin ${DESTDIR}${SYSDIR}/${ARCH}
	-install -d -o bin ${DESTDIR}${SYSDIR}/${ARCH}/OBJ
	install -m 755 *.o ${DESTDIR}${SYSDIR}/${ARCH}/OBJ

clean:
	$(RM) $(OBJS)
