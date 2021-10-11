#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.2
#
LORDER=lorder

CC=/usr/5bin/cc
CFLAGS=-O -DS5EMUL

LIBOBJS=libmai.o libzer.o

liby.a: $(LIBOBJS)
	ar rv liby.a `$(LORDER) *.o | tsort`

install:  liby.a
	install -c -m 644 liby.a $(DESTDIR)/usr/5lib
	ranlib $(DESTDIR)/usr/5lib/liby.a

clean:
	-rm -f *.o liby.a
