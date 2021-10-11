#
# @(#)Makefile	1.1
#
HDR = smerge.h ../ssum/ssum.h

CFLAGS = -O -I../ssum
LDFLAGS =
LINTFLAGS = -I../ssum

OBJS =	y.tab.o lex.yy.o sindex.o env.o intalloc.o subst.o copstr.o \
	amatch.o dosys.o docom.o funny.o \
	decision.o smerge.o names.o ssum.o
CFILES=	y.tab.c lex.yy.c sindex.c env.c intalloc.c subst.c copstr.c \
	amatch.c dosys.c docom.c funny.c \
	decision.c smerge.c ../ssum/names.c ../ssum/ssum.c

.DEFAULT:
	sccs get -G$@ $(@D)/SCCS/s.$(@F) 

smerge: $(OBJS)
	$(CC) $(LDFLAGS) -o smerge $(OBJS) -ll

y.tab.c y.tab.h: gram.y
	yacc -vd gram.y

y.tab.o: y.tab.c defs.h
	$(CC) -g -c y.tab.c

lex.yy.o: lex.yy.c y.tab.h defs.h
	$(CC) -g -c lex.yy.c

lex.yy.c: lex.l
	lex lex.l

names.o: ../ssum/names.c
	$(CC) $(CFLAGS) -c ../ssum/names.c

ssum.o: ../ssum/ssum.c ../ssum/ssum.h
	$(CC) $(CFLAGS) -c ../ssum/ssum.c

lint.out: $(CFILES) $(HDR)
	lint $(LINTFLAGS) $(CFILES) > lint.out 2>&1

tags: $(CFILES) $(HDR)
	ctags $(CFILES) $(HDR) /usr/lib/lint/llib-lc

clean:
	rm -f core $(OBJS) lint.out lex.yy.c y.output y.tab.c y.tab.h \
	smerge.dit smerge

decision.o:	smerge.h
env.o:	var.h
expand.o:	../ssum/ssum.h
expand.o:	smerge.h
lex.yy.o:	y.tab.h
subst.o:	var.h

smerge.dit: smerge.1
	/usr/doctools/bin/ditroff -t -Tpsc -man smerge.1 > smerge.dit

lwman: smerge.dit
	/usr/doctools/bin/psdit smerge.dit | \
	/usr/doctools/bin/psrev | \
	lpr -Jsmerge.1 -Plw2

preview: smerge.dit
	preview smerge.dit

install: smerge smerge.1
	install smerge $(DESTDIR)/usr/release/bin
	install smerge.1 $(DESTDIR)/usr/release/man
