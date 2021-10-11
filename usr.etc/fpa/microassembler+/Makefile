#
# @(#)Makefile 1.1 92/07/30 SMI;
#

CFLAGS= -c
SRCS= main.c sym.c scan.c ramtable.c assemble.c output.c version.c
OBJS= main.o sym.o scan.o ramtable.o assemble.o output.o version.o

fpas+: ${OBJS}
	cc -o fpas+ ${OBJS}

fpas.r: ${OBJS}
	cc -g -o fpas.r ${OBJS}

main.o: main.c micro.h
	cc $(CFLAGS) main.c

sym.o: sym.c micro.h
	cc $(CFLAGS) sym.c

scan.o: scan.c micro.h
	cc $(CFLAGS) scan.c

ramtable.o: ramtable.c micro.h
	cc $(CFLAGS) ramtable.c

assemble.o: assemble.c micro.h
	cc $(CFLAGS) assemble.c

output.o: output.c micro.h
	cc $(CFLAGS) output.c

version.o: version.c
	cc $(CFLAGS) version.c

install: fpas+

clean:
	rm -f *.o fpas+ core *errs* a.out
