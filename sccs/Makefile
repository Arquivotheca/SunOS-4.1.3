#
# @(#)Makefile 1.1 92/07/30 SMI; from System III
#

.KEEP_STATE:

#if the length of the path BIN changes, adjust the install entry
BIN0=	/usr
BIN=	$(BIN0)/sccs
UCB=	$(BIN0)/ucb
# RELBIN is BIN relative to UCB
RELBIN=	../sccs

SRCDIR= src
HDRDIR= hdr
LIBDIR= lib
LIBPWDIR= libPW
HLP_DIR = helpdir
STRIP= -s

HELPFILES = $(HLP_DIR)/ad $(HLP_DIR)/bd $(HLP_DIR)/cb $(HLP_DIR)/cm \
	$(HLP_DIR)/cmds $(HLP_DIR)/co $(HLP_DIR)/de $(HLP_DIR)/default \
	$(HLP_DIR)/ge $(HLP_DIR)/he $(HLP_DIR)/prs $(HLP_DIR)/rc \
	$(HLP_DIR)/un $(HLP_DIR)/ut $(HLP_DIR)/va $(HLP_DIR)/vc

#	Public directory where help files are stored.
PUB_HELPLIB = $(DESTDIR)$(BIN)/helpdir
LOGFLAGS= -DLOGGING
COPTS= -O
CFLAGS= $(COPTS) $(LOGFLAGS)
LDFLAGS=
LIBES = $(LIBDIR)/comobj.a $(LIBPWDIR)/libPW.a
LOC= $(DESTDIR)$(BIN)

MAKE= make CC=$(CC) COPTS=$(COPTS)

ALL=	admin comb delta get help prs prt rmchg unget val vc what
SRC=	$(SRCDIR)/admin.c $(SRCDIR)/comb.c $(SRCDIR)/delta.c \
	$(SRCDIR)/get.c $(SRCDIR)/help.c $(SRCDIR)/prs.c \
	$(SRCDIR)/prt.c $(SRCDIR)/rmchg.c $(SRCDIR)/unget.c \
	$(SRCDIR)/val.c $(SRCDIR)/vc.c $(SRCDIR)/what.c
OBJ=	admin.o comb.o delta.o get.o help.o prs.o prt.o rmchg.o unget.o \
	val.o vc.o what.o

all:	$(LIBES) $(ALL) $(HELPFILES) $(SRCDIR)/sccsdiff.sh

.INIT headers:
	cd hdr; $(MAKE)

install: all
	install -d -o bin -m 755 $(DESTDIR)$(BIN0)
	install -d -o bin -m 755 $(LOC)
	install $(STRIP) admin $(LOC)
	install $(STRIP) comb $(LOC)
	install $(STRIP) delta $(LOC)
	install $(STRIP) get $(LOC)
	install $(STRIP) help $(LOC)
	install $(STRIP) prs $(LOC)
	install $(STRIP) prt $(LOC)
	install $(STRIP) rmchg $(LOC)/rmdel
	rm -f $(LOC)/rmchg $(LOC)/cdc
	ln $(LOC)/rmdel $(LOC)/cdc
	install $(STRIP) unget $(LOC)
	rm -f $(LOC)/sact
	ln $(LOC)/unget $(LOC)/sact
	install $(STRIP) val $(LOC)
	install -d -o bin -m 755 $(DESTDIR)/usr/old
	install $(STRIP) vc $(DESTDIR)/usr/old/vc
	install $(STRIP) what $(LOC)
	rm -f $(DESTDIR)$(UCB)/what
	ln -s $(RELBIN)/what $(DESTDIR)$(UCB)/what
	install -c $(SRCDIR)/sccsdiff.sh $(LOC)/sccsdiff
	install -d -o bin -m 755 $(PUB_HELPLIB)
	@set -x; cd $(HLP_DIR); for i in `ls | grep -v SCCS`; do \
		install -o bin -g bin -m 644 $$i $(PUB_HELPLIB); \
	 done

admin:	admin.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ admin.o $(LIBES)

comb:	comb.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ comb.o $(LIBES)

delta:	delta.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ delta.o $(LIBES)

get:	get.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ get.o $(LIBES)

help:	help.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ help.o $(LIBES)

prs:	prs.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ prs.o $(LIBES)

prt:	prt.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ prt.o $(LIBES)

rmchg:	rmchg.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ rmchg.o $(LIBES)

unget:	unget.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ unget.o $(LIBES)

val:	val.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ val.o $(LIBES)

vc:	vc.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ vc.o $(LIBES)

what:	what.o $(LIBES)
	$(CC) $(LDFLAGS) -o $@ what.o $(LIBES)

$(OBJ): $(SRCDIR)/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(@:.o=.c)

$(LIBDIR)/comobj.a: FRC
	cd $(LIBDIR); $(MAKE) comobj.a

$(LIBPWDIR)/libPW.a: FRC
	cd $(LIBPWDIR); $(MAKE) libPW.a

FRC:

clean:		# Procedure to clean up unwanted files
	-rm -f *.o $(ALL)
	cd lib; $(MAKE) clean
	cd libPW; $(MAKE) clean
