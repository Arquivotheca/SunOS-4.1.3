#
# @(#)Makefile 1.1 92/07/30 SMI
#

# makefile for loadable module utilities

# ASP is Architecture Specific; modload (specifically, modloadconf.c)
# depends upon the kernel sub-architecture

BIN = modunload modstat
ASP = modload
ALL = ${BIN} ${ASP}

ARCH:sh = arch
KARCH:sh = arch -k

COMMON = modsubr.o
MODLOAD = modload.o modloadconf.o modloadsym.o modloadhdr.o
OBJ = ${MODLOAD} modunload.o modstat.o ${COMMON}
ASOBJ = modloadconf.o

DESTDIR =
BINDIR = /usr/etc
OWNER = root

# Install directory for architecture-specific programs, absolute path
#
ASDIR= /usr/kvm

# Install directory for architecture-specific programs, relative path
#
RELASDIR= ../kvm

CFLAGS = -O

${ASOBJ} := CFLAGS += -U${ARCH} -D${KARCH}

all: ${ALL}

modload: ${MODLOAD} ${COMMON}
	${CC} -o modload ${MODLOAD} ${COMMON} -lkvm

modunload: modunload.o ${COMMON}
	${CC} -o modunload modunload.o ${COMMON}

modstat: modstat.o ${COMMON}
	${CC} -o modstat modstat.o ${COMMON}

install: ${ALL}
	for i in ${BIN}; do \
		${RM} ${DESTDIR}${BINDIR}/$$i; \
		install -c -s -m 755 -o ${OWNER} \
			$$i ${DESTDIR}${BINDIR}/$$i ; \
	done
	for i in ${ASP}; do \
		${RM} ${DESTDIR}${BINDIR}/$$i; \
		install -c -s -m 755 -o ${OWNER} \
			$$i ${DESTDIR}${ASDIR}/$$i; \
		ln -s ${RELASDIR}/$$i ${DESTDIR}${BINDIR}/$$i ; \
	done

clean:
	rm -f ${OBJ} ${ALL}
