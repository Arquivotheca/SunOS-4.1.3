# @(#)Makefile 1.1 7/30/92  Copyright Sun Microsystems
#
#
#       makefile for sun4 fpurel  diagnostic sources
#
#

DBX=-O
# specify DBX=-g for dbx version

COPTS=          $(DBX) -DFPU 
CFLAGS=         -DSUN4

INCLUDES=       fpu.h
OFILES=         datapath.o registers.o \
		instruction.o wstatus.o nack.o \
		assymbly.o fpu-if.o others.o winitfp.o fpu.systest.o \
		fpu_probe.o S.linpack.o D.linpack.o

all:    $(INCLUDES) fpurel

fpurel:	$(OFILES)
	ld  -N -o fpurel /lib/crt0.o $(OFILES) -lc -lm

S.linpack.o: linpack.c
	cc $(DBX) -c linpack.c
	mv linpack.o S.linpack.o

D.linpack.o: linpack.c
	cc -DDP $(DBX) -c linpack.c
	mv linpack.o D.linpack.o

.c.o:
		cc $(COPTS) $(CFLAGS) -c  $*.c

fpu-if.o: fpu-if.s
		as  -P -Ff0 -o fpu-if.o fpu-if.s 
assymbly.o: assymbly.s
		as  -P -Ff0 -o assymbly.o assymbly.s 
others.o: others.s
		as  -P -o others.o others.s


clean:
	rm -f *.o core errs fpurel

#       This is a Unix-level test, so belongs in /usr/diag.

install: all FRC
	install -s fpurel $(DESTDIR)/usr/diag
FRC:
