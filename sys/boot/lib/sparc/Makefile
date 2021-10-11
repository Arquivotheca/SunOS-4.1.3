#
# @(#)Makefile 1.1 92/07/30 SMI
#

MACH=	`mach`
ARCH=	`arch`
CPP=	/lib/cpp -undef -Dmc68000 -D${MACH} -Dsun -Dunix
CC=	/bin/cc -${ARCH}
AS=	as -${MACH}
RM=	rm -f

DESTDIR=
OBJDIR=	..
LIBSA=	../${ARCH}/libsa.a
LIBXX=	../${ARCH}/libxx.a
SYSDIR=	../../..
CPPDEFS= -D${ARCH} "-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"
CPPINCS= -I${SYSDIR}/${ARCH} -I${SYSDIR}
CPPOPTS= ${CPPDEFS} ${CPPINCS}
COPTS=	-O
CFLAGS=	${COPTS} ${CPPOPTS}

SRCS=	getidprom.s ldivt.s lmodt.s probe.s stubs.s
OBJS=	$(SRCS:.s=.o)
LIBS=	$(LIBSA) $(LIBXX)

SOBJS=	${OBJDIR}/srt0.o

.KEEP_STATE:
.PRECIOUS: $(LIBS)

ALL=	${LIBSA} ${SOBJS}

all: ${ALL}
libsa.a: ${LIBSA}
libxx.a: ${LIBXX}

$(LIBS): $(OBJS)
	ar rv $@ $?
	ranlib $@
	${RM} $?

.s.a:
	${CPP} ${CPPOPTS} $*.s > $*.i
	${AS} ${ASFLAGS} $*.i -o $*.o
	${RM} $*.i
.s.o:
	${CPP} ${CPPOPTS} $*.s > $*.i
	${AS} ${ASFLAGS} $*.i -o $*.o
	${RM} $*.i

assym.s: genassym.c
	${CC} ${CPPINCS} -D${ARCH} genassym.c; ./a.out >assym.s; \
        ${RM} -f a.out

# need to build srt0.o as a separate object
${OBJDIR}/srt0.o: assym.s srt0.s
	rm -f tmp.c srt0.i srt0.o
	cp srt0.s tmp.c
	${CC} -E -DBOOTBLOCK ${CFLAGS} ${BOOTOPT} -DLOAD=0x${LOAD} tmp.c > srt0.i
	${AS} srt0.i -o srt0.o
	rm -f tmp.c srt0.i
	mv srt0.o ${OBJDIR}

clean:
	${RM} *.o *.i core a.out assym.s

lint:
install:
depend:
