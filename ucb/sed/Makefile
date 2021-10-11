#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.7
#
#	sed make file

SRCDIR = ../../bin/sed
CFLAGS = -O -I$(SRCDIR)
FILES = sed0.o sed1.o

sed:	$(FILES)
	$(CC) -o sed $(FILES)

sed0.o: $(SRCDIR)/sed0.c $(SRCDIR)/sed.h
	$(CC) -c $(CFLAGS) $(SRCDIR)/sed0.c

sed1.o: $(SRCDIR)/sed1.c $(SRCDIR)/sed.h
	$(CC) -c $(CFLAGS) $(SRCDIR)/sed1.c

install: sed
	install -s sed $(DESTDIR)/usr/ucb

clean:
	rm -f $(FILES) sed
