#
# @(#)Makefile 1.1 92/07/30 SMI
#
#	crt0	Normal C run time startoff
#	mcrt0	C run time start off for profiling, ``prof'' conventions
#	gcrt0	C run time start off for profiling, ``gprof'' conventions
#	f?crt0	C run time start off with sky board initialization.
#
LD= ld
CPP= /lib/cpp
CPPFLAGS= -DLOCORE
COMMON= ../common
CSRCS=  $(COMMON)/mon.c $(COMMON)/mon.h $(COMMON)/gmon.c $(COMMON)/gmon.h
SRCS=   crt0.s mcrt0.s gcrt0.s mcount.s gmcount.s Fcrt1.s Scrt1.s Mcrt1.s \
	Wcrt1.s
OBJS=	crt0.o mcrt0.o gcrt0.o mon.o gmon.o \
	Fcrt1.o Scrt1.o Mcrt1.o Wcrt1.o alloc_pbuf.o
CFLAGS= -O -m68010 -fsoft
ASFLAGS= -10
PRINT= print
INSTALL= install -m 644

all: ${OBJS}

install: ${OBJS}
	$(INSTALL)  crt0.o $(DESTDIR)/usr/lib/crt0.o
	$(INSTALL) mcrt0.o $(DESTDIR)/usr/lib/mcrt0.o
	$(INSTALL) gcrt0.o $(DESTDIR)/usr/lib/gcrt0.o
	$(INSTALL) Fcrt1.o $(DESTDIR)/usr/lib/Fcrt1.o
	$(INSTALL) Scrt1.o $(DESTDIR)/usr/lib/Scrt1.o
	$(INSTALL) Mcrt1.o $(DESTDIR)/usr/lib/Mcrt1.o
	$(INSTALL) Wcrt1.o $(DESTDIR)/usr/lib/Wcrt1.o

crt0.o:	crt0.s
	$(CPP) $(CPPFLAGS) crt0.s >tmp.s
	$(AS) $(ASFLAGS) -o crt0.o tmp.s && rm tmp.s
	$(LD) -x -r crt0.o
	mv a.out crt0.o

mcrt0.o: mcrt0.s mon.o
	$(CPP) $(CPPFLAGS) mcrt0.s >tmp.s
	$(AS) $(ASFLAGS) -o mcrt0.o tmp.s && rm tmp.s
	$(LD) -x -r mcrt0.o mon.o
	mv a.out mcrt0.o

gcrt0.o: gcrt0.s gmon.o
	$(CPP) $(CPPFLAGS) gcrt0.s >tmp.s
	$(AS) $(ASFLAGS) -o gcrt0.o tmp.s && rm tmp.s
	$(LD) -x -r gcrt0.o gmon.o
	mv a.out gcrt0.o

.s.o:
	$(CPP) $(CPPFLAGS) $*.s >tmp.s
	$(AS) $(ASFLAGS) -o $*.o tmp.s && rm tmp.s
	$(LD) -x -r $*.o
	mv a.out $*.o
	
gmon.o: $(COMMON)/gmon.c $(COMMON)/gmon.h gmcount.s alloc_pbuf.o
	$(CC) -S $(CFLAGS) $(COMMON)/gmon.c && cat gmcount.s >>gmon.s
	$(AS) $(ASFLAGS) -o gmon.o gmon.s  && rm gmon.s
	$(LD) -x -r gmon.o alloc_pbuf.o && mv a.out gmon.o
	
mon.o: $(COMMON)/mon.c $(COMMON)/mon.h mcount.s alloc_pbuf.o
	$(CC) -S $(CFLAGS) $(COMMON)/mon.c && cat mcount.s >> mon.s
	$(AS) $(ASFLAGS) -o mon.o mon.s  && rm mon.s
	$(LD) -x -r mon.o alloc_pbuf.o && mv a.out mon.o

alloc_pbuf.o : $(COMMON)/alloc_pbuf.c
	$(CC) -c $(CFLAGS) $(COMMON)/alloc_pbuf.c
	
clean:
	rm -f ${OBJS}
print:
	$(PRINT) $(SRCS)
