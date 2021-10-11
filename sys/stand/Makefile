#
# @(#)Makefile 1.1 92/07/30
#

MACH=	`mach`
ARCH=	`arch`
CC=	/bin/cc -${ARCH}
AS=	as -${MACH}
RM=	rm -f

DESTDIR=
SYSDIR=	..
LIBDIR=	lib
MONDIR= mon
SUBDIRS= ${LIBDIR} 

LIBSA=	${LIBDIR}/libsa.a
SRT0=	${LIBDIR}/srt0.o
LINTOBJ= ${LIBDIR}/llib-lsa.ln
.PRECIOUS: ${LIBSA}

CPPDEFS= -D${ARCH} "-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"
CPPOPTS= ${CPPDEFS} -I${LIBDIR}/common -I${SYSDIR}/${ARCH} -I${SYSDIR}
COPTS=	-O
CFLAGS=	${COPTS} ${CPPOPTS}
LOPTS=	-hxb -n
LTAIL=	egrep -v 'struct/union .* never defined' | \
	egrep -v 'possible pointer alignment problem' ; true

# LOAD is text segment start for booted user programs
LOAD=	4000

PROGS=	copy tpcopy
FILES=	copy.c

all: ${MAKEDIRS} ${PROGS}

${SUBDIRS} ${MONDIR}: FRC
	cd $@; ${MAKE} -e ${MFLAGS}


# Standalone copy program for copying eg, tape->disk
copy:	copy.o ${LIBSA} ${SRT0}
	ld -N -e _start -T ${LOAD} -o $@ ${SRT0} $@.o ${LIBSA} -lc

# bootable from tape
tpcopy:	copy
	cp copy tpcopy

# Test program: sacat (standalone cat)
sacat: sacat.o ${LIBSA} ${SRT0}
	ld -N -e _start -T ${LOAD} -o $@ ${SRT0} $@.o ${LIBSA} -lc

${LIBSA} ${SRT0}: FRC
	cd ${LIBDIR}; ${MAKE} -e ${MFLAGS} LOAD=${LOAD} $(@F)

install_h:
	@set -x;for i in ${SUBDIRS} ${MONDIR}; do \
		(cd $$i; ${MAKE} -e ${MFLAGS} DESTDIR=$(DESTDIR) $@); done

depend: Depend
	@for i in ${SUBDIRS}; do \
		(cd $$i; ${MAKE} -e ${MFLAGS} $@); done
Depend: ${FILES}
	@${RM} depend.tmp
	@for i in ${FILES}; do \
		(${CC} -M ${CPPOPTS} $$i >> depend.tmp); done
	@awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
		END { print rec } ' <depend.tmp  > makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w Makefile' >>eddep
	@${RM} Makefile.bak
	@mv Makefile Makefile.bak
	@ed - Makefile.bak < eddep
	@if [ ! -w Makefile.bak ]; then \
		chmod -w Makefile; \
	    fi
	@${RM} eddep makedep depend.tmp

clean: Clean
	@for i in ${SUBDIRS}; do \
		(cd $$i; ${MAKE} -e ${MFLAGS} $@); done
Clean:
	${RM} ${PROGS} sacat *.o core a.out

FRC:

${LINTOBJ}:
	cd ${LIBDIR}; ${MAKE} -e ${MFLAGS} lint

lint: ${LINTOBJ} copy.c
	lint -Dlint ${CPPOPTS} ${LOPTS} copy.c ${LINTOBJ} | ${LTAIL}

install: all
	-mkdir ${DESTDIR}/usr/stand ${DESTDIR}/usr/mdec && \
		chown bin ${DESTDIR}/usr/stand ${DESTDIR}/usr/mdec && \
		chmod 755 ${DESTDIR}/usr/stand ${DESTDIR}/usr/mdec
	install copy ${DESTDIR}/usr/stand
	@for i in ${SUBDIRS}; do \
		(cd $$i; ${MAKE} -e ${MFLAGS} DESTDIR=$(DESTDIR) $@); done

# DO NOT DELETE THIS LINE -- make depend uses it

copy.o: copy.c ../mon/sunromvec.h
