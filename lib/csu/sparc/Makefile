#
# @(#)Makefile 1.1 92/07/30 SMI
#
#	crt0	Normal C run time startoff
#	mcrt0	C run time start off for profiling, ``prof'' conventions
#	gcrt0	C run time start off for profiling, ``gprof'' conventions
#
LD= ld
COMMON= ../common
CSRCS=  $(COMMON)/mon.c $(COMMON)/mon.h $(COMMON)/gmon.c $(COMMON)/gmon.h
SRCS=   crt0.s mcrt0.s gcrt0.s mcount.s gmcount.s
OBJS=	crt0.o mcrt0.o gcrt0.o mon.o gmon.o alloc_pbuf.o \
	misalign_crt0.o misalign_mcrt0.o misalign_gcrt0.o
CFLAGS= -O
ASFLAGS= -P -DLOCORE
PRINT= print
INSTALL= install -m 644

all: ${OBJS}

.DEFAULT:
	sccs get -G$@ $@

install: ${OBJS}
	if test ! -d $(DESTDIR)/lib; then \
		mkdir $(DESTDIR)/lib; \
		chown bin $(DESTDIR)/lib; \
		chmod 755 $(DESTDIR)/lib; \
	fi;
	if test ! -d $(DESTDIR)/usr; then \
		mkdir $(DESTDIR)/usr; \
		chown bin $(DESTDIR)/usr; \
		chmod 755 $(DESTDIR)/usr; \
	fi;
	if test ! -d $(DESTDIR)/usr/lib; then \
		mkdir $(DESTDIR)/usr/lib; \
		chown bin $(DESTDIR)/usr/lib; \
		chmod 755 $(DESTDIR)/usr/lib; \
	fi;
	if test ! -d $(DESTDIR)/usr/lib/misalign; then \
		mkdir $(DESTDIR)/usr/lib/misalign; \
		chown bin $(DESTDIR)/usr/lib/misalign; \
		chmod 755 $(DESTDIR)/usr/lib/misalign; \
	fi;
	$(INSTALL) crt0.o $(DESTDIR)/usr/lib/crt0.o
	$(INSTALL) mcrt0.o $(DESTDIR)/usr/lib/mcrt0.o
	$(INSTALL) gcrt0.o $(DESTDIR)/usr/lib/gcrt0.o
	$(INSTALL) misalign_crt0.o $(DESTDIR)/usr/lib/misalign/crt0.o
	$(INSTALL) misalign_mcrt0.o $(DESTDIR)/usr/lib/misalign/mcrt0.o
	$(INSTALL) misalign_gcrt0.o $(DESTDIR)/usr/lib/misalign/gcrt0.o

crt0.o:	crt0.s
	$(AS) $(ASFLAGS) -o $@ crt0.s
	$(LD) -x -r $@
	mv a.out $@

mcrt0.o: mcrt0.s mon.o
	$(AS) $(ASFLAGS) -o $@ mcrt0.s
	$(LD) -x -r $@ mon.o
	mv a.out $@

gcrt0.o: gcrt0.s gmon.o
	$(AS) $(ASFLAGS) -o $@ gcrt0.s
	$(LD) -x -r $@ gmon.o
	mv a.out $@

misalign_crt0.o:	crt0.s
	$(AS) $(ASFLAGS) -DFIX_ALIGN -o $@ crt0.s
	$(LD) -x -r $@
	mv a.out $@

misalign_mcrt0.o: mcrt0.s mon.o
	$(AS) $(ASFLAGS) -DFIX_ALIGN -o $@ mcrt0.s
	$(LD) -x -r $@ mon.o
	mv a.out $@

misalign_gcrt0.o: gcrt0.s gmon.o
	$(AS) $(ASFLAGS) -DFIX_ALIGN -o $@ gcrt0.s
	$(LD) -x -r $@ gmon.o
	mv a.out $@

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
