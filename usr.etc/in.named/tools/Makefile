#	@(#)Makefile 1.1 92/07/30 SMI from UCB
#
DESTDIR=
CFLAGS=	-O
INCPATH=
LFLAGS=	
LIBS= -lresolv
CC= /bin/cc

# The program itself
#
PROG= nstest	

# Sources
#
SRCS= nstest.c	

# Objects
#
OBJS=	nstest.o


${PROG}: ${OBJS} ${HDRS}
	${CC}  ${LFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	cd nslookup; make ${MFLAGS}

${OBJS}:
	${CC}  ${CFLAGS} ${INCPATH} -c $*.c

install: ${PROG}
	install ${PROG} ${DESTDIR}/usr/etc/${PROG}
	cd nslookup; make ${MFLAGS} DESTDIR=${DESTDIR} install

clean:
	rm -f a.out core errs makefile.bak Makefile.bak ${OBJS} ${PROG}
	cd nslookup; make ${MFLAGS} clean

lint:
	lint ${SRCS}

depend:
	${CC}  -M ${INCPATH} ${SRCS} | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
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

nstest.o: nstest.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
nstest.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
nstest.o: /usr/include/stdio.h /usr/include/arpa/nameser.h
nstest.o: /usr/include/resolv.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
