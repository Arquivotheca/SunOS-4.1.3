#
# @(#)Makefile	1.1
#
SUMOBJ = ssum-main.o ssum.o names.o
SUMSRC = ssum-main.c ssum.c names.c
NAMOBJ = sccsname.o names.o
NAMSRC = sccsname.c names.c
RELOBJ = relpath.o names.o
RELSRC = relpath.c names.c
CFLAGS=-O
LDFLAGS=

.DEFAULT:
	sccs get -G$@ $(@D)/SCCS/s.$(@F)

PRODUCTS= ssum sccsname relpath

all: $(PRODUCTS)

clean:
	rm -f $(SUMOBJ) $(NAMOBJ) $(RELOBJ) $(PRODUCTS)

ssum: $(SUMOBJ)
	$(CC) $(LDFLAGS) -o ssum $(SUMOBJ)

lint.out: ssum.lint sccsname.lint

ssum.lint: $(SUMSRC)
	lint $(SUMSRC) > ssum.lint 2>&1

sccsname.lint: $(NAMSRC)
	lint $(NAMSRC) > sccsname.lint 2>&1

ssum.o ssum-main.o: ssum.h

sccsname: $(NAMOBJ)
	$(CC) $(LDFLAGS) -o sccsname $(NAMOBJ)

relpath: $(RELOBJ)
	$(CC) $(LDFLAGS) -o relpath $(RELOBJ)

ssum.dit: ssum.1
	/usr/doctools/bin/ditroff -t -Tpsc -man ssum.1 > ssum.dit

lwman: ssum.dit
	/usr/doctools/bin/psdit ssum.dit | \
	/usr/doctools/bin/psrev | \
	lpr -Jssum.1 -Plw2

preview: ssum.dit
	preview ssum.dit

install: $(PRODUCTS) ssum.1
	install ssum $(DESTDIR)/usr/release/bin
	install ssum.1 $(DESTDIR)/usr/release/man
	install sccsname $(DESTDIR)/usr/release/bin
	install relpath $(DESTDIR)/usr/release/bin
