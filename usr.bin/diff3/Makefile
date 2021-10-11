#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.1 83/02/10
#
CFLAGS=-O

diff3: diff3.o
	${CC} -o diff3 diff3.o

install: diff3 diff3.sh
	install -s diff3 $(DESTDIR)/usr/lib
	install -c diff3.sh $(DESTDIR)/usr/bin/diff3

clean:
	rm -f diff3.o diff3	
