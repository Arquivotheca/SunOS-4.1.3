#
# @(#)Makefile 1.1 92/07/30 SMI
#

CPPFLAGS += -DSUN_OS_4 -I..
OPTS= -O
CFLAGS= ${OPTS${DBX}}${DBX}

PROGS= mount_tfs umount_tfs unwhiteout lsw
SRC= mount_tfs.c umount_tfs.c unwhiteout.c lsw.c
OBJ= ${SRC:.c=.o}
LINTS = ${SRC:.c=.ln}
LIB = ../libtfs/libtfs.a
LINTLIB = ../libtfs/llib-ltfs.ln

all:	${PROGS}

${PROGS}: $$@.o copyright.o ${LIB}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.o copyright.o ${LIB}

lint:	${LINTS}
	-${LINT.c} mount_tfs.ln ${LINTLIB}
	-${LINT.c} umount_tfs.ln ${LINTLIB}
	-${LINT.c} unwhiteout.ln ${LINTLIB}
	-${LINT.c} lsw.ln ${LINTLIB}

clean:
	rm -f ${PROGS} ${OBJ} ${LINTS} copyright.o core

install:
	install -d ${DESTDIR}/usr/etc
	install -s mount_tfs umount_tfs ${DESTDIR}/usr/etc
	install -d ${DESTDIR}/usr/bin
	install -s unwhiteout lsw ${DESTDIR}/usr/bin
