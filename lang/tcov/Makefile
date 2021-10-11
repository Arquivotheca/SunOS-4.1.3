#
# @(#)Makefile 1.1 92/07/30 SMI
#

SRCS = tcov.c bb_link.c
OBJS = tcov bb_link.o
CPP_FLAGS = 
CFLAGS = -O ${CPP_FLAGS}

all:	${OBJS}

tcov:	tcov.c
	${CC} ${CFLAGS} -o tcov tcov.c

bb_link.o:  bb_link.c
	${CC} ${CFLAGS} -c bb_link.c

install:  ${OBJS}
	install -s -m 555 tcov ${DESTDIR}/usr/bin/tcov
	install -m 644 bb_link.o ${DESTDIR}/usr/lib/bb_link.o

clean:
	rm -f tcov bb_link.o

FRC:

depend: $(SRCS) FRC
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CC) -M $(CPP_FLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
tcov.o: tcov.c
tcov.o: /usr/include/stdio.h
tcov.o: /usr/include/ctype.h
bb_link.o: bb_link.c
bb_link.o: /usr/include/stdio.h
bb_link.o: /usr/include/signal.h
bb_link.o: /usr/include/sys/types.h
bb_link.o: /usr/include/sys/sysmacros.h
bb_link.o: /usr/include/sys/file.h
bb_link.o: /usr/include/sys/fcntl.h
bb_link.o: /usr/include/strings.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
