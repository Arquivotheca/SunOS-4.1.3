#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3 1.11
#
#	spell make file

#	Note:  In using the -f flag it is assumed that either
#	both the host and the target machines need the -f, or
#	neither needs it.  If one needs it and the other does
#	not, it is assumed that the machine that does not need
#	it will treat it appropriately.

CFLAGS= -O

all: spellprog hlista hlistb hstop spellin spellprog hashmake \
	hashcheck

spellprog: hash.h  huff.h spellprog.c hash.c hashlook.c huff.c malloc.c
	$(CC) $(CFLAGS) spellprog.c hash.c hashlook.c huff.c malloc.c -o spellprog

spellin: hash.h huff.h spellin.c huff.c
	$(CC) $(CFLAGS) spellin.c huff.c -o spellin

hashcheck: hash.h huff.h hashcheck.c hash.c huff.c
	$(CC) $(CFLAGS) hashcheck.c hash.c huff.c -o hashcheck

hashmake: hash.h hashmake.c hash.c
	$(CC) $(CFLAGS) hashmake.c hash.c -o hashmake

htemp1:	words local extra hashmake
	cat words local extra | ./hashmake >htemp1

hlista: american hashmake spellin htemp1
	./hashmake <american |sort -u - htemp1 >htemp2
	./spellin `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hlista
	rm htemp2

hlistb: british hashmake spellin htemp1
	./hashmake <british |sort -u - htemp1 >htemp2
	./spellin `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hlistb
	rm htemp2

hstop:	stop spellin hashmake
	./hashmake <stop | sort -u >htemp2
	./spellin `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hstop
	rm htemp2

install: spell.sh compress.sh spellprog spellin hashcheck hashmake hlista \
	hlistb hstop words
	install -d -m 755 ${DESTDIR}/usr/dict
	install -d -m 755 ${DESTDIR}/usr/lib/spell
	install -m 755 spell.sh $(DESTDIR)/usr/bin/spell
	install -m 755 -c compress.sh $(DESTDIR)/usr/lib/spell/compress
	install -s spellprog $(DESTDIR)/usr/lib/spell
	install -s spellin $(DESTDIR)/usr/lib/spell
	install -s hashcheck $(DESTDIR)/usr/lib/spell
	install -s hashmake $(DESTDIR)/usr/lib/spell
	install -c -m 644 hlista $(DESTDIR)/usr/lib/spell
	install -c -m 644 hlistb $(DESTDIR)/usr/lib/spell
	install -c -m 644 hstop $(DESTDIR)/usr/lib/spell
	install -c -m 644 words $(DESTDIR)/usr/dict

clean:	
	rm -f *.o
	rm -f spell spellprog spellin hashmake hlist* hstop
	rm -f htemp1 htemp2 hashcheck
