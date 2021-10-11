#	@(#)libns.mk 1.1 92/07/30 SMI
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
ROOT =
CC = /usr/5bin/cc
LIBDIR = .
USRLIB = $(ROOT)/usr/lib
INCDIR = $(ROOT)/usr/include
DEFS = -Dsun3
LIBNAME = libns.a
LLIB = ns
LINTLIB = llib-l$(LLIB).ln
# LOG=-DLOGGING -DLOGMALLOC
PROFILE=
DEBUG=
CFLAGS=-O $(DEFS) $(DEBUG) $(LOG) $(PROFILE)
SRC= ind_data.c nsblock.c nsports.c nsrports.c \
	rtoken.c astoa.c stoa.c ns_comm.c nslog.c canon.c spipe.c \
	logmalloc.c ns_findp.c ns_getaddr.c ns_getblock.c ns_initaddr.c \
	ns_verify.c ns_error.c ns_errlist.c ns_info.c ns_sendpass.c \
	attconnect.c rfrequest.c negotiate.c getoken.c netname.c \
	swtab.c uidmap.c ns_syntax.c rfs_up.c rfrcv.c ns_xdr.c mount_subr.c

FILES =\
	$(LIBNAME)(ind_data.o)\
	$(LIBNAME)(nsblock.o)\
	$(LIBNAME)(nsports.o)\
	$(LIBNAME)(nsrports.o)\
	$(LIBNAME)(rtoken.o)\
	$(LIBNAME)(stoa.o)\
	$(LIBNAME)(astoa.o)\
	$(LIBNAME)(ns_comm.o) \
	$(LIBNAME)(nslog.o) \
	$(LIBNAME)(canon.o) \
	$(LIBNAME)(spipe.o) \
	$(LIBNAME)(logmalloc.o)\
	$(LIBNAME)(ns_getaddr.o)\
	$(LIBNAME)(ns_findp.o)\
	$(LIBNAME)(ns_getblock.o)\
	$(LIBNAME)(ns_initaddr.o)\
	$(LIBNAME)(ns_verify.o)\
	$(LIBNAME)(ns_error.o)\
	$(LIBNAME)(ns_errlist.o)\
	$(LIBNAME)(ns_info.o)\
	$(LIBNAME)(ns_sendpass.o)\
	$(LIBNAME)(attconnect.o)\
	$(LIBNAME)(rfrequest.o)\
	$(LIBNAME)(negotiate.o)\
	$(LIBNAME)(getoken.o)\
	$(LIBNAME)(netname.o)\
	$(LIBNAME)(uidmap.o)\
	$(LIBNAME)(rfs_up.o)\
	$(LIBNAME)(ns_syntax.o)\
	$(LIBNAME)(swtab.o)\
	$(LIBNAME)(rfrcv.o)\
	$(LIBNAME)(ns_xdr.o)\
	$(LIBNAME)(mount_subr.o)

lib:	$(LIBNAME) 
	ranlib $(LIBNAME)

debug:
	make -f libns.mk LIBNAME=libnsdb.a DEBUG="-g -DDEBUG -DLOGGING -DLOGMALLOC" lib
lint:
	lint -uax -DLOGGING -o $(LLIB) $(SRC)
install: lib
	cp $(LIBNAME) $(USRLIB)
	ranlib $(USRLIB)/$(LIBNAME)
uninstall:
	-rm $(USRLIB)/$(LIBNAME)

.PRECIOUS:	$(LIBNAME)

$(LIBNAME):	$(FILES)

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(LIBNAME)

#### dependencies now follow

$(LIBNAME)(nsports.o): stdns.h nsports.h nsdb.h $(INCDIR)/rfs/nsaddr.h nslog.h $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(nsrports.o): stdns.h nsports.h nsdb.h $(INCDIR)/rfs/nsaddr.h nslog.h \
	$(INCDIR)/rfs/pn.h $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(rtoken.o): stdns.h nsdb.h
$(LIBNAME)(ind_data.o): stdns.h nslog.h $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(nsblock.o): nslog.h nsdb.h stdns.h $(INCDIR)/rfs/nserve.h
$(LIBNAME)(ns_comm.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/nsaddr.h nslog.h stdns.h nsports.h
$(LIBNAME)(nslog.o): nslog.h
$(LIBNAME)(astoa.o): $(INCDIR)/rfs/nsaddr.h
$(LIBNAME)(stoa.o): $(INCDIR)/rfs/nsaddr.h
$(LIBNAME)(ns_getaddr.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/nsaddr.h
$(LIBNAME)(ns_findp.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/nsaddr.h
$(LIBNAME)(ns_getblock.o): $(INCDIR)/rfs/nserve.h
$(LIBNAME)(ns_initaddr.o): $(INCDIR)/rfs/nserve.h
$(LIBNAME)(ns_verify.o): $(INCDIR)/rfs/nserve.h
$(LIBNAME)(ns_sendpass.o): $(INCDIR)/rfs/nserve.h
$(LIBNAME)(attconnect.o): $(INCDIR)/rfs/pn.h
$(LIBNAME)(rfrequest.o): $(INCDIR)/rfs/pn.h $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(negotiate.o): $(INCDIR)/rfs/pn.h $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(getoken.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/cirmgr.h\
	$(INCDIR)/rfs/rfsys.h
$(LIBNAME)(netname.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/cirmgr.h\
	$(INCDIR)/rfs/rfsys.h
$(LIBNAME)(swtab.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/cirmgr.h\
	$(INCDIR/rfs/pn.h
$(LIBNAME)(uidmap.o): idload.h $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/rfsys.h
$(LIBNAME)(rfs_up.o): $(INCDIR)/rfs/nserve.h $(INCDIR)/rfs/rfsys.h
$(LIBNAME)(ns_syntax.o): $(INCDIR)/rfs/nserve.h
$(LIBNAME)(rfrcv.o): $(INCDIR)/sys/tiuser.h
$(LIBNAME)(ns_xdr.o): $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(canon.o): $(INCDIR)/rfs/ns_xdr.h
$(LIBNAME)(mount_subr.o): $(INCDIR)/rfs/ns_xdr.h $(INCDIR)/rfs/nserve.h\
	$(INCDIR)/sys/tiuser.h $(INCDIR)/sys/param.h $(INCDIR)/sys/types.h\
	$(INCDIR)/sys/sysmacros.h $(INCDIR)/sys/ioctl.h $(INCDIR)/sys/vfs.h\
	$(INCDIR)/rfs/cirmgr.h $(INCDIR)/rfs/message.h $(INCDIR)/rfs/rfsys.h\
	$(INCDIR)/stdio.h $(INCDIR)/string.h $(INCDIR)/rfs/nsaddr.h\
	$(INCDIR)/rfs/pn.h $(INCDIR)/rfs/hetero.h $(INCDIR)/sys/mount.h\
	$(INCDIR)/sys/conf.h $(INCDIR)/sys/stropts.h $(INCDIR)/sys/fcntl.h
