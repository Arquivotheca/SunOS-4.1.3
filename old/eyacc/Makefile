#
# @(#)Makefile 1.1 92/08/05 SMI; from UCB 4.1 03/01/81
#
CFLAGS=-O
INSTALLDIR = ${DESTDIR}/usr/old

eyacc: ey0.o ey1.o ey2.o ey3.o ey4.o ey5.o
	${CC} -o eyacc -z ey0.o ey1.o ey2.o ey3.o ey4.o ey5.o

.c.o:
	${CC} -O -c $*.c

clean:
	-rm -f *.o eyacc

install: eyacc
	install -s -c eyacc ${INSTALLDIR}/eyacc

print:
	@ls -l | pr
	@pr READ_ME makefile
	@pr ey.h ey0.c ey1.c ey2.c ey3.c ey4.c ey5.c

ey1.o:	ey.h
ey2.o:	ey.h
ey3.o:	ey.h
ey4.o:	ey.h
