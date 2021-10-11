#
# @(#)Makefile 1.1 92/07/30 SMI
#
CPPFLAGS += -DSUN_OS_4 -I..
OPTS = -O
CFLAGS = ${OPTS${DBX}}${DBX}

SRC =	abspath.c cmdname.c copy_file.c err.c extract_name.c fftruncate.c \
	find_substr.c hash.c hashutil.c hostname.c list.c listelem.c \
	nse_fchdir.c nse_malloc.c nse_mtab.c nse_rpc.c \
	nse_tfs_mount.c parse_brname.c parse_filenam.c path-util.c \
	searchlink.c streq_func.c tfs_client.c tfs_rpc.c tfs_xdr.c \
	fully_qualify.c nse_log.c stdio.c
OBJ = ${SRC:.c=.o}
LINTS = ${SRC:.c=.ln}
LIBRARY = libtfs.a
LINTLIB = llib-ltfs.ln

${LIBRARY}: ${OBJ}
	ar cr ${LIBRARY} ${OBJ}
	ranlib ${LIBRARY}

lint:	lintcheck ${LINTLIB}

#
# The 'lintcheck' rule checks for internal consistency in the library, and
# the ${LINTLIB} rule creates a lint library that programs can be lint'ed
# against.
#
lintcheck: ${LINTS}
	${LINT.c} ${LINTS}

${LINTLIB}: ${SRC}
	${LINT.c} -Ctfs ${SRC}

clean:
	rm -f ${LIBRARY} ${OBJ} ${LINTS} ${LINTLIB} core

install:
