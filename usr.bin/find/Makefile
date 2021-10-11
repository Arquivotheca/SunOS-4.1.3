#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.3 85/06/01
#

DESTDIR=
BINDIR=$(DESTDIR)/usr/bin
LIBDIR=$(DESTDIR)/usr/lib/find

OWNER= bin
CFLAGS=	-O

# binaries installed in BINDIR
BIN=	find

# binaries installed in LIBDIR
LIB=	bigram code

# scripts installed in LIBDIR
LIB2=	updatedb

ALL= $(BIN) $(LIB) $(LIB2)

.KEEP_STATE:

all: $(ALL)

install: $(ALL)
	install -d -o $(OWNER) -m 755 $(BINDIR)
	install -s $(BIN) $(BINDIR)
	install -d -o $(OWNER) -m 755 $(LIBDIR)
	install -s $(LIB) $(LIBDIR)
	install $(LIB2) $(LIBDIR)

clean:
	$(RM) a.out core $(LIB) $(BIN)
