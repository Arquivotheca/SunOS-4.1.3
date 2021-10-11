#
# @(#)Makefile 1.1 92/07/30 SMI;
#

# libm/m68k machine-dependent makefile

FPDEF=	-DF68881

.SUFFIXES:	.S .S~ .il .il~

CPP=	/lib/cpp

# makes occur in .., so M68KINCLUDE is relative to ..
M68KINCLUDE=    -I../../../lib/libc/crt/m68k -I../../../lib/libc/sys/common/m68k
INCLUDE=        -I../../../../lib/libc/crt/m68k -I../../../../lib/libc/sys/common/m68k

LIB=	libm.a
LIBP=	libm_p.a

# directory for objects from machine-independent sources
MIDIR=	midir

IL=	libm.il

M68KSRCS=	../libm.il ../libm3s.S ../libm3d.S ../libm2s.S ../libm2d.S
SRCS=		dependent.il _swapFLAGS.S

M68KOBJS=	libm3s.o libm3d.o libm2s.o libm2d.o
OBJS=		_swapFLAGS.o

all:	$(SRCS) $(M68KSRCS) $(IL) $(OBJS) $(M68KOBJS) $(LIB)

.S~.S:
	sccs get -G$@ $@

.il~.il:
	sccs get -G$@ $@

$(IL):	dependent.il ../libm.il
	cat dependent.il ../libm.il | $(CPP) -P $(FPDEF) | sed 'y/;/\n/' > libm.il
	
$(OBJS):
	$(CC) $(ASFLAGS) -c -DPROF $(FPDEF) $(IL) $(INCLUDE) `basename $@ .o`.S -o ../$@
	${LD} -X -r ../$@
	mv a.out midir/profiled/$@
	$(CC) $(ASFLAGS) -c $(FPDEF) $(IL) $(INCLUDE) `basename $@ .o`.S -o ../$@
	${LD} -x -r ../$@
	rm ../$@
	mv a.out $@

$(M68KOBJS):
	cd .. ; $(CC) $(ASFLAGS) -c -DPROF $(FPDEF) $(IL) $(M68KINCLUDE) `basename $@ .o`.S
	${LD} -X -r ../$@
	mv a.out midir/profiled/$@
	cd .. ; $(CC) $(ASFLAGS) -c $(FPDEF) $(IL) $(M68KINCLUDE) `basename $@ .o`.S
	${LD} -x -r ../$@
	rm ../$@
	mv a.out $@

$(LIB):	$(OBJS) $(M68KOBJS)
	ar cru $(LIB) $(OBJS) $(M68KOBJS)

clean:
	rm -rf a.out core errs $(LIB) $(LIBP) $(OBJS) $(M68KOBJS) $(IL) $(MIDIR)

libm3s.o:	../libm3s.S ../../../../lib/libc/crt/m68k/DEFS.h
libm3d.o:	../libm3d.S ../../../../lib/libc/crt/m68k/DEFS.h
libm2s.o:	../libm2s.S ../../../../lib/libc/crt/m68k/DEFS.h
libm2d.o:	../libm2d.S ../../../../lib/libc/crt/m68k/DEFS.h
_swapFLAGS.o:	midir/_swapFLAGS.o _swapFLAGS.S ../../../../lib/libc/crt/m68k/DEFS.h
