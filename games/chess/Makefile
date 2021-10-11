#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=
CFLAGS=	-O
CC= /bin/cc
OBJS=	agen.o bheur.o book.o bplay.o clock.o data.o init.o io.o \
	mater.o play.o savres.o setup.o stat.o stdin.o wheur.o wplay.o
CFILES=	agen.c bheur.c book.c bplay.c clock.c data.c init.c io.c \
	mater.c play.c savres.c setup.c stat.c stdin.c wheur.c wplay.c
MACHLIB=libmachdep.a
SUBDIR=	m68k sparc

all: chess chess.book

chess: ${OBJS} ${MACHLIB}
	$(CC) ${CFLAGS} -o chess ${OBJS} ${MACHLIB}

${OBJS}: old.h

${MACHLIB}: FRC
	case `mach` in \
	mc68010|mc68020) \
		cd m68k; ${MAKE} ${MFLAGS};; \
	sparc) \
		cd sparc; ${MAKE} ${MFLAGS};; \
	esac

install: chess chess.book
	install -s chess ${DESTDIR}/usr/games/chess
	install chess.book ${DESTDIR}/usr/games/lib/chess.book
clean:
	rm -f ${OBJS} chess chess.book
	for i in ${SUBDIR}; \
	do \
		(cd $$i; ${MAKE} clean;) \
	done

FRC:
