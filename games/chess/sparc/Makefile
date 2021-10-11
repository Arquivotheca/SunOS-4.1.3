#
# @(#)Makefile 1.1 92/07/30 SMI
#

OBJS=	attb.o attw.o bgen.o bmove.o ctrl.o qsort.o wgen.o wmove.o
SFILES=	attb.s attw.s bgen.s bmove.s ctrl.s qsort.s wgen.s wmove.s
MACHLIB=../libmachdep.a
ASFLAGS=-P

${MACHLIB}: ${OBJS}
	ar cr ${MACHLIB} ${OBJS}
	ranlib ${MACHLIB}

clean:
	rm -f ${OBJS} ${MACHLIB}
