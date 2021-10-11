#	@(#)Makefile	1.1	(Berkeley)	7/30/92
#
DESTDIR=
CFLAGS=	-O
CC= /bin/cc

.DEFAULT:
	sccs get -G$@ $@

all:	cno phx ching.sh macros hexagrams

cno:	cno.c
	$(CC) -O -o cno cno.c

phx:	phx.c
	$(CC) -O -o phx phx.c

install: all
	install -c ching.sh ${DESTDIR}/usr/games/ching
	install -d -m 755 ${DESTDIR}/usr/games/lib/ching.d
	install -c cno ${DESTDIR}/usr/games/lib/ching.d/cno
	install -c phx ${DESTDIR}/usr/games/lib/ching.d/phx
	install -c macros ${DESTDIR}/usr/games/lib/ching.d/macros
	install -c hexagrams ${DESTDIR}/usr/games/lib/ching.d/hexagrams

clean:
	rm -f cno phx a.out core *.s *.o
