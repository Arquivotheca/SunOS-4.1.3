#       @(#)Makefile 1.1 7/30/92 Copyright Sun Microsystems
#
#       makefile for sun3 fparel  diagnostic sources
#
#       @(#)Makefile 1.1 2/8/85 SMI
#


CC	= cc
CHOWN	= chown
ARCH	= `arch`

CFLAGS=         -ffpa -Dmc68020 -DTOKEN_BUF_SIZE=256 -I/usr/include/sundev

OFILES=         memory_3x.o memory.o reg_ram.o lock.o register.o simpleins.o \
		ust_reg.o wbranching.o wdp.o woperation.o wlwf.o \
		wstatus.o nack2.o wtiming.o contexts.o pointers.o \
		ptr_incdec.o fpa.systest.o S.linpack.o D.linpack.o \
		fparel.o fparel_util.o signal.o

all:    fparel

memory_3x.o: fpa3x.h
memory.o   : fpa.h fpa.def.h
signal.o   : fpcrttypes.h


S.linpack.o: linpack.c
	${CC} -ffpa -c linpack.c
	mv linpack.o S.linpack.o

D.linpack.o: linpack.c
	${CC} -DDP -ffpa -c linpack.c
	mv linpack.o D.linpack.o


fparel: $(OFILES) 
	$(LINK.c) -o $@ $(OFILES)

clean:
		rm -f *.o errs fparel

install:      fparel
	if test ! -d $(DESTDIR)/usr ; then \
	        mkdir $(DESTDIR)/usr && \
	        $(CHOWN) bin ${DESTDIR}/usr && \
	          chmod 755 ${DESTDIR}/usr ; \
	        fi
	if test ! -d $(DESTDIR)/usr/etc ; then \
	        mkdir $(DESTDIR)/usr/etc && \
	        $(CHOWN) bin ${DESTDIR}/usr/etc && \
	          chmod 755 ${DESTDIR}/usr/etc ; \
	        fi
	if test ! -d $(DESTDIR)/usr/etc/fpa ; then \
	        mkdir $(DESTDIR)/usr/etc/fpa && \
	        $(CHOWN) bin ${DESTDIR}/usr/etc/fpa && \
	          chmod 755 ${DESTDIR}/usr/etc/fpa ; \
	        fi
	if test -d $(DESTDIR)/usr/etc/fpa ; then \
	        install -s fparel  $(DESTDIR)/usr/etc/fpa/fparel ; \
	fi

.c.o:
		$(CC) $(CFLAGS) -c $*.c

