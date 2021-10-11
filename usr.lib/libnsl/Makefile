#	@(#)Makefile 1.1 92/07/30 SMI; from S5R3N8 */

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)libnsl:nsl/nsl.mk	1.9"
# 
# Network services library
#

DESTDIR=
LIBNAME= libnsl.a
USRLIB=/usr/lib
INCRT=/usr/include
CC=/usr/5bin/cc
CFLAGS= -O 

SRC= t_accept.c t_bind.c t_connect.c t_error.c t_close.c\
	 t_getinfo.c t_getstate.c t_listen.c t_look.c\
	 t_rcv.c t_rcvconnect.c t_rcvdis.c t_snd.c t_snddis.c\
	 t_unbind.c t_optmgmt.c\
	 t_rcvudata.c t_rcvuderr.c t_sndudata.c t_sndrel.c t_rcvrel.c\
	 t_alloc.c  t_free.c t_open.c t_sync.c\
	 _dummy.c _errlst.c _data.c _conn_util.c _utility.c

INCLUDES=  	$(INCRT)/sys/param.h\
		$(INCRT)/sys/types.h\
		$(INCRT)/sys/errno.h\
		$(INCRT)/sys/stream.h\
		$(INCRT)/sys/stropts.h\
		$(INCRT)/nettli/tihdr.h\
		$(INCRT)/nettli/timod.h\
		$(INCRT)/nettli/tiuser.h\
		$(INCRT)/sys/signal.h

FILES = $(LIBNAME)(t_accept.o)\
	 $(LIBNAME)(t_bind.o)\
	 $(LIBNAME)(t_connect.o)\
	 $(LIBNAME)(t_error.o)\
	 $(LIBNAME)(t_close.o)\
	 $(LIBNAME)(t_getinfo.o)\
	 $(LIBNAME)(t_getstate.o)\
	 $(LIBNAME)(t_listen.o)\
	 $(LIBNAME)(t_look.o)\
	 $(LIBNAME)(t_rcv.o)\
	 $(LIBNAME)(t_rcvconnect.o)\
	 $(LIBNAME)(t_rcvdis.o)\
	 $(LIBNAME)(t_snd.o)\
	 $(LIBNAME)(t_snddis.o)\
	 $(LIBNAME)(t_unbind.o)\
	 $(LIBNAME)(t_optmgmt.o)\
	 $(LIBNAME)(t_rcvudata.o)\
	 $(LIBNAME)(t_rcvuderr.o)\
	 $(LIBNAME)(t_sndudata.o)\
	 $(LIBNAME)(t_sndrel.o)\
	 $(LIBNAME)(t_rcvrel.o)\
	 $(LIBNAME)(t_alloc.o)\
	 $(LIBNAME)(t_free.o)\
	 $(LIBNAME)(t_open.o)\
	 $(LIBNAME)(t_sync.o)\
	 $(LIBNAME)(_dummy.o)\
	 $(LIBNAME)(_errlst.o)\
	 $(LIBNAME)(_data.o)\
	 $(LIBNAME)(_conn_util.o)\
	 $(LIBNAME)(_utility.o)


all:	$(LIBNAME)

lib:	$(LIBNAME) 
	ranlib $(LIBNAME)

.PRECIOUS:	$(LIBNAME)

$(LIBNAME):	$(FILES)

$(SRC):	$(INCLUDES)

#
# XXX - there maybe ought to be a real rule for this.
#
$(INCLUDES):

install:  all
	install -d -m 755 $(DESTDIR)/usr/lib
	install -o bin -g bin -m 644 $(LIBNAME) $(DESTDIR)$(USRLIB)
	ranlib $(DESTDIR)$(USRLIB)/$(LIBNAME)

clean:
	rm -f *.o core .make.state $(LIBNAME)
