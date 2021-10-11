#
#	@(#)Makefile.com 1.1 92/07/30 SMI
#
# Common makefile for all versions of kadb
#
# The work gets done in the sys/kadb/$(ARCH) directory. We assume the
# Makefile in that directory has already defined $(ARCH) and $(MACH).
#

INSTALLDIR=	${DESTDIR}/usr/stand
SYSDIR= ../..
COMDIR= ../common
ARCHHDR= ${SYSDIR}/${ARCH}
ADIR=	${SYSDIR}/${ARCH}/debug
AOBJ=	${ADIR}/debug.o
ATAGS=	(cd ${ADIR}; ${MAKE} -e tags.list | sed -e 's,../../,${SYSDIR}/,g')
ASTAGS=	(cd ${ADIR}; ${MAKE} -e stags.list | sed -e 's,../../,${SYSDIR}/,g')
RELOC=	(cd ${ADIR}; ${MAKE} -e reloc)
DDIR=	${SYSDIR}/../lang/adb
DHDR=	${DDIR}/common
MACDIR=	${SYSDIR}/adb
MACLIST=`cd ${MACDIR}; ${MAKE} -e maclist`
BOOT=	${SYSDIR}/boot
LIBBOOT=${BOOT}/${ARCH}/libkadb.a

LIBPROM=	${SYSDIR}/${ARCH}/libprom.a
LIBPROMSRCDIR=	${BOOT}/lib/${ARCH}

LIBSA=	${BOOT}/lib/${ARCH}/libsadb.a
CPPINCS= -I. -I${COMDIR} -I${SYSDIR} -I${ARCHHDR} -I${DHDR} -I${DDIR}/${MACH}
CPPOPTS= -DKADB -D${ARCH} ${CPPINCS}
COPTS=	-O
CFLAGS=	${COPTS} ${CPPOPTS}

GREP=	egrep

LIBS=	${LIBBOOT} ${LIBSA} ${ARCHLIBS} -lc

HFILES= ${COMDIR}/pf.h

CFILES= ${COMDIR}/kadb.c ${COMDIR}/genpf.c

OBJ=	kadb.o

all:	kadb

kadb:	ukadb.o pf.o
	${RELOC} > reloc
	${LD} -N -T `cat reloc` -e _start -Bstatic -o $@ ukadb.o pf.o -lc
	${RM} reloc

# ukadb.o is the a.out for all of kadb except the macro file pf.o,
# this makes it is easier to drop in different set of macros.
ukadb.o: ${AOBJ} ${OBJ} adb.o ${LIBBOOT} ${LIBSA} ${ARCHLIBS}
	${LD} -r -o $@ ${AOBJ} ${OBJ} adb.o ${LIBS}

adb.o:	FRC
	@ ${RM} $@
	ln -s ${DDIR}/${MACH}/kadb/kadb.o $@
	cd ${DDIR}/${MACH}/kadb; ${MAKE} -e ${MFLAGS} kadb.o

${LIBBOOT} ${LIBSA} ${AOBJ}: FRC
	cd $(@D); ${MAKE} -e ${MFLAGS} $(@F)

${LIBPROM}: FRC
	cd $(LIBPROMSRCDIR); ${MAKE} -e ${MFLAGS}

# don't strip to make patching `ndbootdev' and `vmunix' variables easier
install: kadb
	install kadb ${INSTALLDIR}

tags:	FRC
	${ATAGS} > tags.list
	echo ${DDIR}/${MACH}/*.[ch] > adbtags.list
	ctags ${HFILES} ${CFILES} `cat tags.list` \
		${BOOT}/${ARCH}/*.[ch] ${BOOT}/os/*.[ch] \
		${BOOT}/lib/${ARCH}/*.[ch] ${BOOT}/lib/common/*.[ch] \
		`cat adbtags.list` ${DDIR}/common/*.[ch]
	${ASTAGS} > stags.list
	$(GREP) 'ENTRY2*\(' `cat stags.list` \
		${BOOT}/${ARCH}/*.s ${BOOT}/lib/${ARCH}/*.s \
		${BOOT}/${MACH}/*.s ${BOOT}/lib/${MACH}/*.s | sed	\
    -e 's|^\(.*\):\(.*ENTRY(\)\(.*\)\().*\)|\3	\1	/^\2\3\4$$/|'	\
    -e 's|^\(.*\):\(.*ENTRY2(\)\(.*\),\(.*\)\().*\)|\3	\1	/^\2\3,\4\5$$/\
\4	\1	/^\2\3,\4\5$$/|' >> tags;				\
	sort -u -o tags tags
	${RM} tags.list adbtags.list stags.list

clean:
	${RM} a.out *.o genpf pf.c errs tags.list tags reloc
	@ cd ${ADIR}; ${MAKE} -e ${MFLAGS} clean

FRC:

genpf:	${COMDIR}/genpf.c
	cc -o $@ ${COMDIR}/genpf.c

pf.c:	genpf
	cd ${MACDIR}; ${MAKE} -e ${MFLAGS}
	genpf ${MACLIST}

pf.o:	pf.c
	${CC} -c ${CFLAGS} -DKERNEL $<

kadb.o: ${COMDIR}/kadb.c
	${CC} -c ${CFLAGS} ${COMDIR}/kadb.c

depend: ${CFILES} ${HFILES}
	${RM} makedep
	for i in ${CFILES}; do \
			(${CC} -M ${CPPOPTS} $$i >> makedep); done
	@echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w Makefile' >>eddep
	@${RM} Makefile.bak
	@mv Makefile Makefile.bak
	@ed - Makefile.bak < eddep
	@if [ ! -w Makefile.bak ]; then \
		chmod -w Makefile; \
	    fi
	@${RM} eddep makedep

