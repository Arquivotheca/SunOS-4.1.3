#
# @(#)Makefile 1.1 92/07/30 SMI
#

CPPFLAGS += -DSUN -DTFSDEBUG -DSUN_OS_4 -I..
OPTS = -O
CFLAGS = ${OPTS${DBX}}${DBX}

PROG =	tfsd
HDRS =	headers.h subr.h sysdep.h tfs.h tfs_lists.h vnode.h
SRC =	tfsd.c tfs_attrs.c tfs_backlink.c tfs_dirs.c tfs_dupcache.c \
	tfs_fd.c tfs_lists.c tfs_lookup.c tfs_mount.c tfs_ntree.c \
	tfs_ops.c tfs_printf.c tfs_server.c tfs_subr.c tfs_swap.c \
	tfs_trace.c tfs_update.c tfs_vnodeops.c tfs_wcache.c tfs_xdr.c \
	copyright.c
OBJ = ${SRC:.c=.o}
LINTS = ${SRC:.c=.ln}
LIB = ../libtfs/libtfs.a
LINTLIB = ../libtfs/llib-ltfs.ln

.INIT:	${HDRS}

${PROG}: ${OBJ} ${LIB}
	${CC} ${CFLAGS} ${LDFLAGS} -o ${PROG} ${OBJ} ${LIB}

lint: ${LINTS}
	${LINT.c} ${LINTS} ${LINTLIB}

clean:
	rm -f ${PROG} ${OBJ} ${LINTS} core

install:
	install -d ${DESTDIR}/usr/etc
	install -s ${PROG} ${DESTDIR}/usr/etc
