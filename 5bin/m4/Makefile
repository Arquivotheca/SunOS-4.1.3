#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.3
#
YACCRM=-rm
CFLAGS = -O

m4:	m4.o m4ext.o m4macs.o m4y.o
	$(CC) $(LDFLAGS) -o m4 m4.o m4ext.o m4macs.o m4y.o

m4.o:	m4.c m4.h
m4ext.o: m4ext.c
m4macs.o: m4macs.c
m4y.o:	m4y.y

test:
	rtest m4

install: m4
	install -s m4 $(DESTDIR)/usr/5bin

clean:
	-rm -f *.o
	$(YACCRM) -f y.tab.c 
	-rm -f m4
