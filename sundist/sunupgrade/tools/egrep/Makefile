#
# @(#)Makefile 1.100 90/11/01 SMI; from UCB 4.38 85/08/26
#

CFLAGS=	-O $(IFLAGS)
DOPRNT= ../lib/libc/stdio/common
LDFLAGS=
CC= /bin/cc
S5CC= /usr/5bin/cc
CHOWN=	/usr/etc/chown
CLEANFILES= clean egrep egrep.y expr expr.y

.KEEP_STATE:


# YACC programs that live in the current directory, that should be built
# with the 4.2 C compiler, and that need explicit make lines.
#
#
FILES=egrep.y expr.y
SRCPATH=../../../../usr.bin
TOOLS=egrep expr

all:	egrep expr


egrep:
	${YACC} ${YFLAGS} egrep.y
	${CC} -o $@ ${CFLAGS} -Bstatic  -w y.tab.c
	rm -f y.tab.c

expr:
	${YACC} ${YFLAGS} expr.y
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} -Bstatic y.tab.c
	/usr/bin/rm -f y.tab.c

${FILES}:	${SRCPATH}/$$@
	cp ${SRCPATH}/$@ .

install:
	for i in $(TOOLS); do \
		install -s -m 755 $$i ${DESTDIR}/toolkit/$$i; done

clean:
	for i in ${CLEANFILES}; do \
		rm -f $$i; done

