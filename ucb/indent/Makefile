#
# @(#)Makefile 1.1 92/07/30 Copyright 1985 Sun Micro; from UCB 4.2 10/22/82
#
DESTDIR=
CFLAGS=-O2
debug := CFLAGS=	-g

OBJS= indent.o io.o lexi.o parse.o pr_comment.o args.o

indent: ${OBJS}
	${CC} ${CFLAGS} -o indent ${OBJS}
debug: indent

${OBJS}: indent_globs.h indent_codes.h

objsall: ${OBJS}

clean:
	rm -f ${OBJS} indent; sccs clean

install: indent
	install -s indent ${DESTDIR}/usr/bin/indent
.KEEP_STATE:
