#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.3
#
CFLAGS = -O
FILES = main.o sub1.o sub2.o header.o

.DEFAULT:
	sccs get $@

lex:	$(FILES) y.tab.o
	$(CC) $(LDFLAGS) $(FILES) y.tab.o -o lex

$(FILES): ldefs.c
main.o:	  once.c
y.tab.c:  parser.y
	$(YACC) parser.y

install: lex ncform nrform
	install -s lex $(DESTDIR)/usr/bin
	-mkdir -p $(DESTDIR)/usr/share/lib/lex && \
		chown bin $(DESTDIR)/usr/share/lib/lex && \
		chmod 755 $(DESTDIR)/usr/share/lib/lex 
	install -c -m 644 ncform $(DESTDIR)/usr/share/lib/lex
	install -c -m 644 nrform $(DESTDIR)/usr/share/lib/lex
	-rm -rf ${DESTDIR}/usr/lib/lex
	ln -s ../share/lib/lex ${DESTDIR}/usr/lib/lex

clean:
	rm -f *.o lex y.tab.c
