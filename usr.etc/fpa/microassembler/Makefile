#
# @(#)Makefile 1.1 92/07/30 SMI; Copyr 1986 by Sun Microsystems, Inc.
#

CFLAGS=-g -c
CC= /bin/cc
SRCS= main.c sym.c scan.c assemble.c output.c
OBJS= main.o sym.o scan.o assemble.o output.o


fpas: ${OBJS}
	$(CC) -g -o fpas ${OBJS}

main.o: main.c micro.h
	$(CC) $(CFLAGS) main.c

sym.o: sym.c micro.h
	$(CC) $(CFLAGS) sym.c

scan.o: scan.c micro.h
	$(CC) $(CFLAGS) scan.c

assemble.o: assemble.c micro.h
	$(CC) $(CFLAGS) assemble.c

output.o: output.c micro.h
	$(CC) $(CFLAGS) output.c

install: fpas

clean:
	rm -f *.o fpas core errs a.out
