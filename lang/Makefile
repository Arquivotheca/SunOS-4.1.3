#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=
CPP= /lib/cpp
CPPFLAGS=
CFLAGS=	${CPPFLAGS} -O -DBROWSER
CC=/usr/ucb/cc
d-mc68020=m68k
d-mc68010=m68k
d-sparc=sparc
CPU=$(d$(TARGET_MACH))
MAKE= make CC="$(CC)"

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	adb as cflow compile cpp cxref c2 dbx pcc inline iropt ld lint \
	ucbcflow ucbcxref rtld tcov m4 vroot yacc

# Programs that are really shell scripts
# Currently, they all go (it goes) in /usr/bin
#
SCRIPT= lorder.sh

cpu-sun2 = m68k
cpu-sun3 = m68k
cpu-sun3x = m68k
cpu-sun4 = sparc
cpu-sun4d = sparc
CPU = $(cpu$(TARGET_ARCH))

# C programs that live in the current directory and do not need
# explicit make lines. Listed by the directories in which they are installed
#
USR_BIN= ranlib nm size strip ar
USR_BINSRC= ranlib.c nm.c size.c strip.c
UCB= symorder
UCBSRC= symorder.c ar.c
LIBCB_DIR= browser/cblib
LIBCB= $(LIBCB_DIR)/$(CPU)/libcb.a

STD= ${USR_BIN} ${UCB}
SRCS= ${USR_BINSRC} ${UCBSRC}

all:	${SUBDIR} ${STD} 

${SUBDIR}: FRC
	cd $@; $(MAKE) ${MFLAGS} AS="${AS}" CPU="${CPU}"

xall xm68k xsunrise: ${STD}
	for i in ${SUBDIR}; do \
	    (echo $$i; $(MAKE) $@ ${MFLAGS} AS="${AS}"); \
	done

${STD}: $(LIBCB)
	${CC} ${CFLAGS} -o $@ $@.c ${LDFLAGS}\
			-I$(LIBCB_DIR)/src -DBROWSER $(LIBCB)

$(LIBCB): FRC
	cd $(@D); $(MAKE) $(@F)

# not in release, but a useful tool
objdump:
	${CC} ${CFLAGS} -o $@ $@.c ${LDFLAGS}

# make target directories if necessary
# this should not, in general, be necessary
install: ${STD} ${SUBDIR} ${SCRIPT}
	install -d -o bin -m 755 ${DESTDIR}/usr/bin
	install -d -o bin -m 755 ${DESTDIR}/usr/lib
	install -d -o bin -m 755 ${DESTDIR}/usr/ucb
	-for i in ${SUBDIR}; do \
	    (cd $$i; $(MAKE) ${MFLAGS} CPU=${CPU} DESTDIR=${DESTDIR} install); \
	 done
	-for i in ${SCRIPT}; do \
	    ( install -c $$i ${DESTDIR}/usr/bin/`basename $$i .sh`); \
	 done
	-for i in ${USR_BIN};do (install -s $$i ${DESTDIR}/usr/bin/$$i); done
	-for i in ${UCB};    do (install -s $$i ${DESTDIR}/usr/ucb/$$i); done

clean:
	rm -f a.out core *.s *.o errs
	@set -x; for i in ${SUBDIR}; do (cd $$i; $(MAKE) ${MFLAGS} clean CPU="${CPU}" ); done
	rm -f ${STD}
	cd browser/cblib/$(CPU); make clean

FRC:

depend: ${SRCS}
	rm -f makedep
	for i in ${SRCS}; do \
	    ( ${CPP} -M ${CPPFLAGS} $$i | \
		sed -e 's/\.o:/:/' -e '/sysmacros/d' >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

ranlib: ranlib.c
ranlib: /usr/include/sys/types.h
ranlib: /usr/include/sys/stat.h
ranlib: /usr/include/ar.h
ranlib: /usr/include/ranlib.h
ranlib: /usr/include/a.out.h
ranlib: /usr/include/sys/exec.h
ranlib: /usr/include/stdio.h
ar: ar.c
ar: /usr/include/sys/param.h
ar: /usr/include/machine/param.h
ar: /usr/include/sys/signal.h
ar: /usr/include/vm/faultcode.h
ar: /usr/include/sys/types.h
ar: /usr/include/sys/stat.h
ar: /usr/include/sys/time.h
ar: /usr/include/time.h
ar: /usr/include/stdio.h
ar: /usr/include/ar.h
nm: nm.c
nm: /usr/include/sys/types.h
nm: /usr/include/ar.h
nm: /usr/include/stdio.h
nm: /usr/include/ctype.h
nm: /usr/include/a.out.h
nm: /usr/include/sys/exec.h
nm: /usr/include/stab.h
nm: /usr/include/sys/stat.h
size: size.c
size: /usr/include/stdio.h
size: /usr/include/a.out.h
size: /usr/include/sys/exec.h
strip: strip.c
strip: /usr/include/a.out.h
strip: /usr/include/sys/exec.h
strip: /usr/include/signal.h
strip: /usr/include/vm/faultcode.h
strip: /usr/include/stdio.h
strip: /usr/include/sys/file.h
strip: /usr/include/sys/fcntlcom.h
symorder: symorder.c
symorder: /usr/include/stdio.h
symorder: /usr/include/a.out.h
symorder: /usr/include/sys/exec.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
