#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.2 85/08/26
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#

.KEEP_STATE:

DESTDIR=
CFLAGS=	-O
LIBS=

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	adventure backgammon battlestar boggle chess ching cribbage fortune \
	hack hangman hunt mille monop quiz robots snake tool trek

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	arithmetic btlgammon banner bcd bj \
	canfield cfscores craps \
	factor fish moo number \
	ppt primes rain random \
	worm worms wump

canfield rain worm worms := LIBS += -lcurses -ltermcap
primes := LIBS += -lm


all:	$(SUBDIR) $(STD)

$(SUBDIR): FRC
	cd $@ && $(MAKE) $(MFLAGS)

$(STD): $$@.c
	$(CC) $(CFLAGS) -o $@ $@.c $(LIBS)

install: $(STD) backrules.lib FRC
	install -d -o bin -m 755 $(DESTDIR)/usr/games
	install -d -o bin -m 755 $(DESTDIR)/usr/games/lib
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) DESTDIR=$(DESTDIR) $@); done
	install -s -o bin $(STD) $(DESTDIR)/usr/games
	install -o bin -m 644 backrules.lib $(DESTDIR)/usr/games/lib/backrules
	install -o bin -m 666 /dev/null $(DESTDIR)/usr/games/lib/cfscores

clean: FRC
	$(RM) a.out core *.s *.o $(STD)
	for i in $(SUBDIR); do (cd $$i && $(MAKE) $(MFLAGS) $@); done

FRC:

