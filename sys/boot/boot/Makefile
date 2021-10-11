#
# @(#)Makefile 1.1 92/07/30 SMI
#

m-sun2 = sun2
m-sun3 = sun3
m-sun4 = sun4

ARCH = $(m$(TARGET_ARCH))

m-mc68010 = m68k
m-mc68020 = m68k
m-sparc = sparc
m-sunrise = sparc

MACH = $(m$(TARGET_MACH))

CC=	/bin/cc -${MACH}
AS=	as -${MACH}
RM=	rm -f

DESTDIR= /
SYSDIR=	../..
LIBDIR=	../lib
MONDIR= ../mon
LIBSA=	${LIBDIR}/${ARCH}/libsa.a
LIBXX=	${LIBDIR}/libxx.a
SRT0=	../${ARCH}/srt0.o
LINTOBJ= ${LIBDIR}/llib-lsa.ln

.PRECIOUS: ${LIBSA} ${LIBXX}

CPPDEFS= -DVAC -D${ARCH} "-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"
CPPOPTS= ${CPPDEFS} -I${LIBDIR}/common -I${SYSDIR}/${ARCH} -I${SYSDIR}
COPTS=	-O
CFLAGS=	${COPTS} ${CPPOPTS}

LDIR=	/usr/lib/lint
LINT1=	${LDIR}/lint1
LCOPTS=	-C -Dlint ${CPPOPTS} -DLOAD=0x${LOAD} 
LOPTS=	-hxb -n
LTAIL=	egrep -v 'struct/union .* never defined' | \
	egrep -v 'possible pointer alignment problem' ; true

# BRELOC is text segment start for final stage (2nd or 3rd stage) boots
# Note that this must be higher than the kernel's edata and should be
# higher than the kernel's bss so the bss can be cleared
BRELOC=1a0000
# LOAD is text segment start for booted user programs
LOAD=4000

PROGS=	tpboot 

SUN2PROGS= ${PROGS} 
SUN3PROGS= ${PROGS} 
SUN4PROGS= ${PROGS}

ALL=	archdep

all: ${ALL}
archdep:
	@case ${ARCH} in \
	sun2)	${MAKE} ${MFLAGS} ${SUN2PROGS};;\
	sun3)	${MAKE} ${MFLAGS} ${SUN3PROGS};;\
	sun4)	sed -e 's/bs=32/bs=64/' \
		-e 's/BRELOC=120000/BRELOC=200000/' Makefile > Makefile.sun4;\
		${MAKE} -e ${MFLAGS} -f Makefile.sun4 ${SUN4PROGS};;\
	esac


# Standalone, generic boot program -- get it from anywhere, it will
# boot anything.  "tpboot" has the a.out header stripped off so you
# can write it on the first file of a tape.
sboot:	tpboot.o readfile.o ${LIBSA} ${SRT0}
	ld -N -e _start -T ${BRELOC} -o $@ ${SRT0} tpboot.o readfile.o ${LIBSA} -lc

tpboot:	sboot
	cp sboot a.out
	strip a.out; dd if=a.out of=$@ ibs=32 skip=1; ${RM} a.out

readfile.o: readfile.c
	${CC} -c ${CFLAGS} -DLOAD=0x${LOAD} readfile.c

tpboot.o: boot.c
	${CC} -c -o $@ ${CFLAGS} -DJUSTASK boot.c

confxy.o: confxx.c
	${CC} -c -o ${LIBDIR}/${ARCH}/$@ ${CFLAGS} -Dxxdriver=xydriver confxx.c

confip.o: confxx.c
	${CC} -c -o $@ ${CFLAGS} -Dxxdriver=ipdriver confxx.c

confsd.o: confxx.c
	${CC} -c -o ${LIBDIR}/${ARCH}/$@ ${CFLAGS} -Dxxdriver=sddriver confxx.c

confpr.o: confpr.c
	${CC} -c -o ${LIBDIR}/${ARCH}/$@ ${CFLAGS} -Dxxdriver=sddriver confpr.c

confxd.o: confxx.c
	${CC} -c -o ${LIBDIR}/${ARCH}/$@ ${CFLAGS} -Dxxdriver=xddriver confxx.c

${SRT0}: FRC
	cd ../${ARCH}; ${MAKE} ${MFLAGS} $(@F)

${LIBSA} ${LIBXX}: FRC
	cd ${LIBDIR}; ${MAKE} ${MFLAGS} $(@F)

# utilities

depend:
	@echo 'making dependencies ...'
	@${RM} depend.tmp
	@for i in *.c; do \
		(${CC} -M ${CPPOPTS} $$i >> depend.tmp); done
	@awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
		END { print rec } ' <depend.tmp  > makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@if [ -w Makefile ]; then \
		ed - Makefile < eddep; \
	    else \
		chmod +w Makefile; \
		ed - Makefile < eddep; \
		chmod -w Makefile; \
	    fi
	@${RM} eddep makedep depend.tmp
	@echo '... done'

clean:
	${RM} ${SUN2PROGS} ${SUN3PROGS} ${SUN4PROGS}
	${RM} *.o *.exe *.ln *.c core a.out sboot

${LINTOBJ}:
	cd ${LIBDIR}; ${MAKE} ${MFLAGS} lint

lint: ${LINTOBJ} boot.c readfile.c
	@${RM} lint.ln
	@-(for i in readfile.c ; do \
		${CC} -E ${LCOPTS} $$i | \
		${LINT1} ${LOPTS} >> lint.ln; done ) 2>&1 | ${LTAIL}
	lint -Dlint ${CPPOPTS} ${LOPTS} boot.c lint.ln ${LINTOBJ} | ${LTAIL}
	@${RM} lint.ln

install_h:
FRC:

install: ${ALL}


# DO NOT DELETE THIS LINE -- make depend uses it

