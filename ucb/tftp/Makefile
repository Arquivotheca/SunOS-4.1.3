#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB     06/12/83
#
ALL=	tftp in.tftpd
DESTDIR=
CFLAGS=-O

all: ${ALL}

tftp:	main.o tftp.o tftpsubs.o
	${CC} main.o tftp.o tftpsubs.o -o tftp

in.tftpd:	in.tftpd.o tftpsubs.o
	${CC} in.tftpd.o tftpsubs.o -o in.tftpd

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?

install:  ${ALL}
	install -s tftp ${DESTDIR}/usr/ucb
	install -s in.tftpd ${DESTDIR}/usr/etc
