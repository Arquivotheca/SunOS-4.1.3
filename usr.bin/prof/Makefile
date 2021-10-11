#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.1 06/02/83
#
# If you don't want to plot, take out the -Dplot and the ref. to plot.a
PLOT = -lplot -Dplot
CFLAGS=-O 
CC= /bin/cc

all: prof

prof:	prof.o
	$(CC) -o prof prof.o $(PLOT)

prof.o: prof.c
	$(CC) -c $(CFLAGS) $(PLOT) prof.c

install: all
	install -s prof $(DESTDIR)/usr/bin

clean :
	rm -f *.o prof
