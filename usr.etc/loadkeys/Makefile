#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1988 Sun Microsystems, Inc.
#

.KEEP_STATE:
.FRC:

# Binaries, where to install them
BINS = loadkeys dumpkeys
BINDIR = $(DESTDIR)/usr/bin

# Keytable files, where to install them
KEYTABLES = us belgium_france germany swiss_german swiss_french uk canada \
	denmark italy netherlands norway portugal spain_latin_america \
	sweden_finland japan korea taiwan layout_81 layout_82 reset \
	germany_5 norway_5 sweden_5 uk_5 italy_5 portugal_5 swiss_french_5 \
	us_5 denmark_5 netherlands_5 spain_5 swiss_german_5 france_5 \
	japan_5 korea_5 taiwan_5

KEYTABLEDIR = $(DESTDIR)/usr/share/lib/keytables

CFLAGS = -O -I/usr/src/include -I/usr/src/sys

all:  $(BINS) $(KEYTABLES)

install: $(BINS) $(KEYTABLES)
	install -d -m 755 $(BINDIR)
	install -d -m 755 $(KEYTABLEDIR)
	install -s $(BINS) $(BINDIR)
	install -m 644 $(KEYTABLES) $(KEYTABLEDIR)
# Type 4 keytables
	rm -f $(KEYTABLEDIR)/layout_00
	ln $(KEYTABLEDIR)/us $(KEYTABLEDIR)/layout_00
	rm -f $(KEYTABLEDIR)/layout_01
	ln $(KEYTABLEDIR)/us $(KEYTABLEDIR)/layout_01
	rm -f $(KEYTABLEDIR)/layout_02
	ln $(KEYTABLEDIR)/belgium_france $(KEYTABLEDIR)/layout_02
	rm -f $(KEYTABLEDIR)/layout_03
	ln $(KEYTABLEDIR)/canada $(KEYTABLEDIR)/layout_03
	rm -f $(KEYTABLEDIR)/layout_04
	ln $(KEYTABLEDIR)/denmark $(KEYTABLEDIR)/layout_04
	rm -f $(KEYTABLEDIR)/layout_05
	ln $(KEYTABLEDIR)/germany $(KEYTABLEDIR)/layout_05
	rm -f $(KEYTABLEDIR)/layout_06
	ln $(KEYTABLEDIR)/italy $(KEYTABLEDIR)/layout_06
	rm -f $(KEYTABLEDIR)/layout_07
	ln $(KEYTABLEDIR)/netherlands $(KEYTABLEDIR)/layout_07
	rm -f $(KEYTABLEDIR)/layout_08
	ln $(KEYTABLEDIR)/norway $(KEYTABLEDIR)/layout_08
	rm -f $(KEYTABLEDIR)/layout_09
	ln $(KEYTABLEDIR)/portugal $(KEYTABLEDIR)/layout_09
	rm -f $(KEYTABLEDIR)/layout_0a
	ln $(KEYTABLEDIR)/spain_latin_america $(KEYTABLEDIR)/layout_0a
	rm -f $(KEYTABLEDIR)/layout_0b
	ln $(KEYTABLEDIR)/sweden_finland $(KEYTABLEDIR)/layout_0b
	rm -f $(KEYTABLEDIR)/layout_20
	ln $(KEYTABLEDIR)/japan $(KEYTABLEDIR)/layout_20
	rm -f $(KEYTABLEDIR)/layout_0c
	ln $(KEYTABLEDIR)/swiss_french $(KEYTABLEDIR)/layout_0c
	rm -f $(KEYTABLEDIR)/layout_0d
	ln $(KEYTABLEDIR)/swiss_german $(KEYTABLEDIR)/layout_0d
	rm -f $(KEYTABLEDIR)/layout_0e
	ln $(KEYTABLEDIR)/uk $(KEYTABLEDIR)/layout_0e
	rm -f $(KEYTABLEDIR)/layout_10
	ln $(KEYTABLEDIR)/korea $(KEYTABLEDIR)/layout_10
	rm -f $(KEYTABLEDIR)/layout_11
	ln $(KEYTABLEDIR)/taiwan $(KEYTABLEDIR)/layout_11
# Type 5 keytables
	rm -f $(KEYTABLEDIR)/layout_21
	ln $(KEYTABLEDIR)/us_5 $(KEYTABLEDIR)/layout_21
	rm -f $(KEYTABLEDIR)/layout_22
	ln $(KEYTABLEDIR)/us_5 $(KEYTABLEDIR)/layout_22
	rm -f $(KEYTABLEDIR)/layout_23
	ln $(KEYTABLEDIR)/france_5 $(KEYTABLEDIR)/layout_23
	rm -f $(KEYTABLEDIR)/layout_24
	ln $(KEYTABLEDIR)/denmark_5 $(KEYTABLEDIR)/layout_24
	rm -f $(KEYTABLEDIR)/layout_25
	ln $(KEYTABLEDIR)/germany_5 $(KEYTABLEDIR)/layout_25
	rm -f $(KEYTABLEDIR)/layout_26
	ln $(KEYTABLEDIR)/italy_5 $(KEYTABLEDIR)/layout_26
	rm -f $(KEYTABLEDIR)/layout_27
	ln $(KEYTABLEDIR)/netherlands_5 $(KEYTABLEDIR)/layout_27
	rm -f $(KEYTABLEDIR)/layout_28
	ln $(KEYTABLEDIR)/norway_5 $(KEYTABLEDIR)/layout_28
	rm -f $(KEYTABLEDIR)/layout_29
	ln $(KEYTABLEDIR)/portugal_5 $(KEYTABLEDIR)/layout_29
	rm -f $(KEYTABLEDIR)/layout_2a
	ln $(KEYTABLEDIR)/spain_5 $(KEYTABLEDIR)/layout_2a
	rm -f $(KEYTABLEDIR)/layout_2b
	ln $(KEYTABLEDIR)/sweden_5 $(KEYTABLEDIR)/layout_2b
	rm -f $(KEYTABLEDIR)/layout_2c
	ln $(KEYTABLEDIR)/swiss_french_5 $(KEYTABLEDIR)/layout_2c
	rm -f $(KEYTABLEDIR)/layout_2d
	ln $(KEYTABLEDIR)/swiss_german_5 $(KEYTABLEDIR)/layout_2d
	rm -f $(KEYTABLEDIR)/layout_2e
	ln $(KEYTABLEDIR)/uk_5 $(KEYTABLEDIR)/layout_2e
	rm -f $(KEYTABLEDIR)/layout_2f
	ln $(KEYTABLEDIR)/korea_5 $(KEYTABLEDIR)/layout_2f
	rm -f $(KEYTABLEDIR)/layout_30
	ln $(KEYTABLEDIR)/taiwan_5 $(KEYTABLEDIR)/layout_30
	rm -f $(KEYTABLEDIR)/layout_31
	ln $(KEYTABLEDIR)/japan_5 $(KEYTABLEDIR)/layout_31

clean:
	-rm -rf $(BINS) core a.out y.tab.c
