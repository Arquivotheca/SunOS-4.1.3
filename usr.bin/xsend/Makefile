#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 83/06/30
#
CFLAGS=	-O
DESTDIR=
ALL=	enroll xsend xget

all:	${ALL}

enroll:	enroll.o lib.o
	${CC} enroll.o lib.o -lmp -o enroll

xget:	xget.o lib.o
	${CC} xget.o lib.o -lmp -o xget

xsend:	xsend.o lib.o
	${CC} xsend.o lib.o -lmp -o xsend

enroll.o xget.o xsend.o lib.o:	xmail.h

list:
	@pr -2 -w124 m.h xmail.h enroll.c xsend.c xget.c lib.c Makefile

install: enroll xsend xget
	-mkdir ${DESTDIR}/var/spool/secretmail && \
		chown bin ${DESTDIR}/var/spool/secretmail && \
		chmod 777 ${DESTDIR}/var/spool/secretmail
	for i in ${ALL}; do install -s $$i ${DESTDIR}/usr/bin; done

lint:
	@lint -bhnux ${COPTS} enroll.c
	@lint -bhnux ${COPTS} xsend.c 
	@lint -bhnux ${COPTS} xget.c
	@lint -bhnux ${COPTS} lib.c 

clean:
	rm -f *.o yacc.* y.tab.c ${ALL}
