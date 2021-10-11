#
# @(#)Makefile 1.1 92/07/30 (Sun)
#

CPPFLAGS = -DLIBPFC -I${LIBPCDIR} -I${LIBI77DIR}  -I${LIBF77DIR} 
CFLAGS = -O ${CPPFLAGS}

LIB = ${DESTDIR}/usr/lib
UCBDIR = ${DESTDIR}/usr/ucb
LIBPCDIR = ../libpc
LIBF77DIR = ../libF77
LIBI77DIR = ../libI77

INSTALL= install -c -m 644
RM = rm -f
RANLIB= ranlib
AR= ar
CPP= /lib/cpp

PSRCS =	${LIBPCDIR}/PCSTART.c
FSRCS = ${LIBF77DIR}/main.c

IHDRS = ${LIBI77DIR}/f_errno.h ${LIBI77DIR}/fiodefs.h
FHDRS = ${LIBF77DIR}/main.c
PHDRS = ${LIBPCDIR}/PCSTART.c ${LIBPCDIR}/h00vars.h \
	${LIBPCDIR}/libpc.h ${LIBPCDIR}/whoami.h
HDRS = ${IHDRS} ${FHDRS} ${PHDRS}

SRCS= ${PSRCS} ${FSRCS} ${HDRS}

POBJS =	PCSTART.o
FOBJS = main.o

DIRS =	profiled

.DEFAULT:
	sccs get -G$@ $@

libpfc.a libpfc_p.a: ${DIRS} ${POBJS} ${FOBJS}
	cd profiled; ${AR} cru ../libpfc_p.a ${POBJS} ${FOBJS}
	${RANLIB} libpfc_p.a
	${AR} cru libpfc.a ${POBJS} ${FOBJS}
	${RANLIB} libpfc.a

${DIRS}:
	-mkdir $@

${POBJS}:
	${CC} -p ${CFLAGS} -c -o tmp.o ${LIBPCDIR}/`basename $@ .o`.c
	-${LD} -X -r -o profiled/$@ tmp.o
	${CC} ${CFLAGS} -c -o tmp.o ${LIBPCDIR}/`basename $@ .o`.c
	-${LD} -x -r -o $@ tmp.o

${FOBJS}:
	${CC} -p ${CFLAGS} -c -o tmp.o ${LIBF77DIR}/`basename $@ .o`.c
	-${LD} -X -r -o profiled/$@ tmp.o
	${CC} ${CFLAGS} -c -o tmp.o ${LIBF77DIR}/`basename $@ .o`.c
	-${LD} -x -r -o $@ tmp.o

install: libpfc.a libpfc_p.a
	${INSTALL} libpfc_p.a ${LIB}/libpfc_p.a
	${RANLIB} ${LIB}/libpfc_p.a
	${INSTALL} libpfc.a ${LIB}/libpfc.a
	${RANLIB} ${LIB}/libpfc.a

clean:
	${RM} libpfc.a libpfc_p.a *.o profiled/*.o a.out lpr tmp core errs

print:
	ls -l >lst
	${UCBDIR}/vprint lst Makefile *.h [A-Z][A-Z]*.[cs] [a-z][a-z]*.[cs]
	${RM} lst

grind: ${SRCS}
	${UCBDIR}/ctags -v *.h *.c | sort -f >index
	${UCBDIR}/vgrind -t -x index >lpr
	${UCBDIR}/vgrind -t -n Makefile >>lpr
	${RM} index
	${UCBDIR}/vgrind -t *.h *.c >>lpr
	${UCBDIR}/vpr -t lpr

depend:	${SRCS}
	rm -f makedep
	for i in ${PSRCS} ${FSRCS}; do \
	    ( ${CPP} -M ${CPPFLAGS} $$i >> makedep ); \
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
# DEPENDENCIES MUST END AT END OF FILE
PCSTART.o: ../libpc/PCSTART.c
PCSTART.o: /usr/include/signal.h
PCSTART.o: ../libpc/h00vars.h
PCSTART.o: /usr/include/stdio.h
PCSTART.o: ../libpc/whoami.h
PCSTART.o: ../libpc/libpc.h
main.o: ../libF77/main.c
main.o: /usr/include/stdio.h
main.o: /usr/include/signal.h
main.o: ../libF77/../libI77/fiodefs.h
main.o: /usr/include/stdio.h
main.o: ../libF77/../libI77/f_errno.h
main.o: /usr/include/errno.h
main.o: /usr/include/sys/errno.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
