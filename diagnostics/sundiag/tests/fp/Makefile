#	Makefile for fp.
#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
ALL=	fputest mc68881 fpatest
ARCH=`arch`
DESTDIR=
REV=NEW
OS=SVR4
#compile with OS= for 4.x version.
INCLUDES=./fp.h $(INCDIR)/sdrtns.h ./fpa3x_def.h \
	./fpa3x_msg.h $(ONLINEINC)/libonline.h

DBX    =-O
# specify DBX=-g for dbx version

CFLAGS = $(DBX) -I${INCDIR} -D${ARCH} -D$(REV) -D$(OS)
FFLAGS = -u -O -c
MLIB   =-lm
KVMLIB = -lkvm
ELFLIB = -lelf
#compile with ELFLIB= for 4.x version.
NETLIB =-lnsl

FPUOBJS=fputest.o S.linpack.o D.linpack.o fpu_probe.o \
	fpu-if.o fpu-test.o

FPAOBJS=fpa3x.o fpa3x_math.o fpa3x_dev.o fpa3x_msg.o \
	fpa3x_menu.o
LINOBJS=S.linpack.o D.linpack.o

.KEEP_STATE:
all:	${INCLUDES} ${ALL}

fpatest: ${FPAOBJS} ${LINOBJS}
	@if [ ${ARCH} = "sun3" ] || [ ${ARCH} = "sun3x" ]; then \
		echo "TARGET operation is fpatest"; \
		${CC} ${FPAOBJS} ${LINOBJS} ${SDLIBS} ${MLIB} -o fpatest; \
	fi

${FPAOBJS}:  ${FPASRCS}
	@if [ ${ARCH} != "sun4" ]; then \
		${CC} ${CFLAGS} -ffpa -c $*.c; \
	fi

fputest: ${FPUOBJS} fptest.c fp.h $(INCDIR)/sdrtns.h
	@if [ ${ARCH} = "sun4" ]; then \
		echo "TARGET operation is fputest"; \
		${CC} $(CFLAGS) -DFPU -c fptest.c; \
		if [ $(OS) ]; then \
                        if [ $(OS) = "SVR4" ]; then \
				${CC} fptest.o ${FPUOBJS} $(SDLIBS) ${MLIB} $(NETLIB) \
					$(KVMLIB) $(ELFLIB) -o fputest; \
			fi \
		else \
			${CC} fptest.o ${FPUOBJS} $(SDLIBS) ${MLIB} $(KVMLIB) -o fputest; \
		fi \
	fi

fputest.o: fputest.c $(INCDIR)/sdrtns.h
	@if [ ${ARCH} = "sun4" ]; then \
		${CC} ${CFLAGS} -DFPU -DSUN4 -c fputest.c; \
	fi

S.linpack.o: linpack.c
	@if [ $(ARCH) = "sun3" ]; then \
		${CC} ${CFLAGS} -ffpa -c linpack.c; \
	elif [ $(ARCH) = "sun4" ]; then \
		${CC} ${CFLAGS} -c linpack.c; \
	fi
	mv linpack.o S.linpack.o

D.linpack.o: linpack.c
	@if [ $(ARCH) = "sun3" ]; then \
		${CC} ${CFLAGS} -DDP -ffpa -c linpack.c; \
	elif [ $(ARCH) = "sun4" ]; then \
		${CC} ${CFLAGS} -DDP -c linpack.c; \
	fi
	mv linpack.o D.linpack.o

fpu_probe.o: fpu_probe.c
	@if [ $(ARCH) = "sun4" ]; then \
		cc $(CFLAGS) $(FPUOPTS) -c fpu_probe.c; \
	fi

fpu-if.o:  fpu-if.s svr4fpu-if.s
	@if [ $(ARCH) = "sun4" ]; then \
		if [ $(OS) ]; then \
			if [ $(OS) = "SVR4" ]; then \
				as -o fpu-if.o svr4fpu-if.s; \
			fi \
		else \
			as -Ff0 -o fpu-if.o fpu-if.s; \
		fi \
	fi

fpu-test.o:  fpu-test.s svr4fpu-test.s
	@if [ $(ARCH) = "sun4" ]; then \
		if [ $(OS) ]; then \
			if [ $(OS) = "SVR4" ]; then \
				as -o fpu-test.o svr4fpu-test.s; \
			fi \
		else \
			as -Ff0 -o fpu-test.o fpu-test.s; \
		fi \
	fi

mc68881: fp.h fptest.c $(INCDIR)/sdrtns.h
	@if [ $(ARCH) = "sun3" ]  || [ ${ARCH} = "sun3x" ]; then \
		echo "TARGET operation is mc68881"; \
		${CC} ${CFLAGS} -DMC68881 -DMCNSOFT -f68881 -c fptest.c; \
		${CC} -f68881 fptest.o $(SDLIBS) ${MLIB} $(KVM) -o mc68881; \
	fi

install: all
	@if [ $(ARCH) = "sun3" ]  || [ ${ARCH} = "sun3x" ]; then \
	  if [ $(DESTDIR) ]; then \
	    set -x; \
	    install -s fpatest mc68881 ${DESTDIR}; \
	  else \
	    set -x; \
	    install fpatest mc68881 ../../bin; \
	  fi \
	elif [ $(ARCH) = "sun4" ]; then \
	  if [ $(DESTDIR) ]; then \
	    set -x; install -s fputest ${DESTDIR}; \
	  else \
	    set -x; install fputest ../../bin; \
	  fi \
	fi

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?
include ../../include/Makefile.end
