#
# @(#)Makefile 1.1 92/08/05 SMI; from UCB 4.3 83/03/10
#
CFLAGS=-O

.DEFAULT:
	sccs get $@

all: compact uncompact

compact: compact.o tree.o
	${CC} compact.o tree.o -o compact

uncompact: uncompact.o tree.o
	${CC} uncompact.o tree.o -o uncompact

compact.o uncompact.o tree.o: compact.h

install: compact uncompact ccat.sh
	install -s compact ${DESTDIR}/usr/old
	install -s uncompact ${DESTDIR}/usr/old
	install -c -m 755 ccat.sh ${DESTDIR}/usr/old/ccat

clean:
	rm -f compact.o uncompact.o tree.o compact uncompact
