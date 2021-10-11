#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.3
#
UBIN=$(DESTDIR)/usr/bin
ULIB=$(DESTDIR)/usr/lib
CFLAGS=-O
INS=install
OFILES = y1.o y2.o y3.o y4.o

yacc:	$(OFILES) yaccpar
	$(CC) $(LDFLAGS) -o yacc $(OFILES)

$(OFILES):	dextern files

install:   yacc yaccpar
	$(INS) -s yacc $(UBIN)
	$(INS) -c -m 644 yaccpar $(ULIB)/yaccpar

clean:
	-rm -f $(OFILES) yacc
