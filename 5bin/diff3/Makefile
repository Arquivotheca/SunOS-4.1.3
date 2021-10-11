#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.5
#
#	diff3 make file

IFLAG =
CFLAGS = -O
LDFLAGS = $(IFLAG)
CC = /usr/5bin/cc
MAKE = make

.DEFAULT:
	sccs get $@

diff3prog: diff3prog.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o diff3prog diff3prog.c

install: diff3prog diff3.sh
	install -c diff3.sh $(DESTDIR)/usr/5bin/diff3
	install -s diff3prog $(DESTDIR)/usr/5lib

clean:
	rm -f diff3prog
