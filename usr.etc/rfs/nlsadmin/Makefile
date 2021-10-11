#
#       @(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright 1987 Sun Microsystems 
#
#       Copyright (c) 1984 AT&T
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.

#ident	"@(#)nlsadmin:nlsadmin.mk	1.5.1.2"

DESTDIR=
BINS=nlsadmin
CC = /usr/5bin/cc
CFLAGS = -O
LDFLAGS =
LDLIBS=
SOURCE = main.c util.c nlsaddr.c nstoa.c nlsstr.c
OBJECTS= $(SOURCE:%.c=%.o)
FRC=

.KEEP_STATE:

.INIT: nlsadmin.h nsaddr.h

all: $(BINS)

$(BINS): $(OBJECTS) 
	$(CC) $(CFLAGS) -o $(BINS) $(LDFLAGS) $(OBJECTS)

install: $(BINS)
	install -d $(DESTDIR)/usr/bin
	install -s $(BINS) $(DESTDIR)/usr/bin

clean:
	$(RM) .make.state $(BINS) *.o core

FRC:
