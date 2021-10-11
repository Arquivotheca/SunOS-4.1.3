
#
# @(#)Makefile 1.100 90/11/01 SMI; from UCB 4.38 85/08/26
#
# Makefile from /usr/src/sundist/sunupgrade/tools/ls5

CFLAGS=	-O $(IFLAGS)

# I need to keep a copy locally for testing
DOPRNT=	.
LDFLAGS=
CC= /bin/cc
S5CC= /usr/5bin/cc
CHOWN=	/usr/etc/chown


# files found in /usr/5bin ---------------------------------------------
#
BIN5PATH=../../../../5bin

BIN5FILES=ls
# ----------------------------------------------------------------------

CLEANFILES=ls.c ls5

.KEEP_STATE:

# Programs built "static" for upgrade toolset
#

all:	getfiles nonstd

getfiles:	${BIN5FILES}

nonstd: ${BIN5FILES} 

# cp files from source directories ---------------------------------------------

${BIN5FILES:%=%.c}:	${BIN5PATH}/$$@
	cp ${BIN5PATH}/$@ .


# compile them ----------------------------------------------------------------

# 5bin/ls
ls:	ls.c
	${S5CC} -o ls5 ${CFLAGS} -Bstatic  -DSINGLE -DS5EMUL ls.c -lcurses

install:
	install -s -m 755 ls5 ${DESTDIR}/toolkit/ls5


clean:
	for i in ${CLEANFILES}; do \
		rm -f $$i; done
