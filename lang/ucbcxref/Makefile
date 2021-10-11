#
# @(#)Makefile 1.1 92/07/30 SMI
#
CFLAGS = -O -I../cxref -DCXREF -DFLEXNAMES

all:	cxref

# CXREF

cxref:	cxr.o
	$(CC) $(FFLAG) $(LDFLAGS) cxr.o -o cxref

cxr.o:	../cxref/src/cxr.c ../cxref/src/owner.h
	$(CC) -c $(CFLAGS) ../cxref/src/cxr.c

# UTILITIES

install:	all
	install -s cxref $(DESTDIR)/usr/bin

shrink:
	-rm -f *.o

clean: shrink
	-rm -f cxref
