#
#       @(#)  Makefile 1.1 92/07/30
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#
#	dorfs make file

DESTDIR=

.MAKE.STATE:

all:	dorfs

dorfs: dorfs.sh
	cp dorfs.sh dorfs
	chmod 755 dorfs

install: dorfs dorfs.sh
	install -m 755 -o root -g bin dorfs $(DESTDIR)/usr/bin

clean:
	rm -f dorfs
