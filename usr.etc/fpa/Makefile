#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.27 83/07/06
#
DESTDIR=
CFLAGS=	-O
CC= /bin/cc

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	microassembler microassembler+ microcode microcode+ download version

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	

# Programs that must run setuid to root
#
SETUID=	

# C programs that live in the current directory and need explicit make lines.
#
NSTD=

all:	${SUBDIR} ${STD} ${NSTD}

${SUBDIR}: FRC
	cd $@; $(MAKE) ${MFLAGS}

#${STD}:
#	$(CC) ${CFLAGS} -o $@ $@.c

install: ${STD} ${NSTD}
	-mkdir ${DESTDIR}/usr/etc/fpa && chown bin ${DESTDIR}/usr/etc/fpa && \
		chmod 755 ${DESTDIR}/usr/etc/fpa
	@set x;for i in ${SUBDIR}; do \
		(cd $$i; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install); done
#	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/etc/fpa/$$i); done
#	for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/etc/fpa/$$i); done
#	for i in ${SETUID}; do (chown root ${DESTDIR}/usr/etc/fpa/$$i; \
		chmod 4755 ${DESTDIR}/usr/etc/fpa/$$i); done

clean:
	rm -f a.out core *.s *.o
	@set x;for i in ${SUBDIR}; do (cd $$i; $(MAKE) ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

FRC:

depend:
	cat </dev/null >x.c
	@set x;for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

# DO NOT DELETE THIS LINE -- make depend uses it

