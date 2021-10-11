#
# @(#)Makefile 1.1 92/07/30 SMI;
#
CFLAGS=-O
GET= sccs get

fpaversion: fpaversion.c fpa.h values.h
	${CC} fpaversion.c -o fpaversion

values.h: ../microcode/constants get_validation_file
	sed -f get_validation_file ../microcode/constants > values.h

clean:
	rm -f fpaversion values.h core

install: fpaversion
	install -s fpaversion ${DESTDIR}/usr/etc/fpa/fpaversion

get_validation_file:
	$(GET) $@
