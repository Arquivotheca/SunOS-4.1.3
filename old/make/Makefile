#
# @(#)Makefile 1.1 92/08/05 SMI; from S5R2 1.7
#

# The rules.c file can be modified locally for people who still like
#	things like fortran.

LDFLAGS =
INSDIR = $(DESTDIR)/usr/old
LIBS =
CFLAGS = -O -DBSD -DSCCSDIR

OBJECTS =  \
	main.o \
	doname.o \
	misc.o \
	files.o \
	rules.o \
	dosys.o \
	gram.o \
	dyndep.o \
	prtmem.o

all:  make

make:  $(OBJECTS)
	$(CC) -o make $(LDFLAGS) $(OBJECTS) $(LIBS)

gram.c:	gram.y

gram.o: gram.c

$(OBJECTS):  defs

install: all
	install -c -s make $(INSDIR)

clean:
	-rm -f *.o a.out core errs make gram.c

tags:	NOW
	ctags *.[ch]

NOW:
	;

.DEFAULT:
	sccs get $@
