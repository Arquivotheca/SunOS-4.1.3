#
#	@(#)Makefile 1.1 92/07/30 SMI
#
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libns:libns.mk	1.10"
# This makefile makes libns.a, which is the library for
# the name server library.
# NOTE: this library is not for general use.  It is put
# 	in /usr/lib ONLY for the convenience of the
#	commands that use it.
#
CC=/usr/5bin/cc
DESTDIR=
LIBNAME= libns.a
LLIB= ns
# LOG=-DLOGGING -DLOGMALLOC
PROFILE=
DEBUG=
DEFS= -DRFS -D$(TARGET_ARCH:-%=%)
CFLAGS= -O $(DEFS) $(DEBUG) $(LOG) $(PROFILE)

SRC= ind_data.c nsblock.c nsports.c nsrports.c \
	rtoken.c astoa.c stoa.c ns_comm.c nslog.c canon.c spipe.c \
	logmalloc.c ns_findp.c ns_getaddr.c ns_getblock.c ns_initaddr.c \
	ns_verify.c ns_error.c ns_errlist.c ns_info.c ns_sendpass.c \
	attconnect.c rfrequest.c negotiate.c getoken.c netname.c \
	swtab.c uidmap.c ns_syntax.c rfs_up.c rfrcv.c ns_xdr.c

LIBS=${SRC:%.c=${LIBNAME}(%.o)}

.KEEP_STATE:

.INIT: idload.h nsdb.h nslog.h nsports.h stdns.h

${LIBNAME}: $(LIBS)

${LIBNAME}(%.o): %.o
	$(AR) $(ARFLAGS) ${LIBNAME} $*.o

#debug:
#	make LIBNAME=libnsdb.a DEBUG="-g -DDEBUG -DLOGGING -DLOGMALLOC" lib

install: ${LIBNAME}
	install -d -m 755 $(DESTDIR)/usr/lib
	install -m 644 $(LIBNAME) $(DESTDIR)/usr/lib/
	ranlib $(DESTDIR)/usr/lib/${LIBNAME}

clean:
	rm -f $(LIBNAME) *.o

.PRECIOUS:	$(LIBNAME)
