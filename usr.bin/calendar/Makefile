#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.1 83/08/13
#
DESTDIR=
CFLAGS=	-O

calendar: calendar.c
	${CC} ${CFLAGS} -o calendar calendar.c

install: calendar calendar.sh
	install -s calendar $(DESTDIR)/usr/lib
	install -c -m 755 calendar.sh $(DESTDIR)/usr/bin/calendar

clean:
	rm -f calendar
