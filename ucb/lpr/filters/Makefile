#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.1 05/11/83
#
# makefile for line printer filters and associated programs
#
LCFLAGS=-O
CFLAGS=$(LCFLAGS) $(CF) 
LIBDIR=/usr/lib
CXREF=/usr/ucb/ctags -x
DESTDIR=

SRCS=	lpf.c necf.c prxf.c \
	chrtab.c rvsort.c rvcat.c vsort.c vcat.c vpf.c vpsf.c vdmp.c vpltdmp.c \
	vplotf.c vfontinfo.c vwidth.c vfw.c fcvt.c rotate.c rotprt.c \
	vswap.c
PRINTER=	lpf necf prxf vswap
VARIAN=	rvsort rvcat vsort vcat vpf vpsf vdmp vpltdmp vplotf vfontinfo vwidth \
	vfw fcvt rotate rotprt

all:	printer varian

printer: ${PRINTER}

varian:	${VARIAN}

lpf:	lpf.c
	${CC} -o lpf ${CFLAGS} lpf.c

necf:	necf.c
	${CC} -o necf ${CFLAGS} necf.c

prxf:	prxf.c
	${CC} -o prxf ${CFLAGS} prxf.c

rvsort:	rvsort.c
	${CC} -o rvsort ${CFLAGS} rvsort.c

rvcat:	rvcat.c
	${CC} -o rvcat ${CFLAGS} rvcat.c

vsort:	vsort.c
	${CC} -o vsort ${CFLAGS} vsort.c

vcat:	vcat.c
	${CC} -o vcat ${CFLAGS} vcat.c

vpf:	vpf.c
	${CC} -o vpf ${CFLAGS} vpf.c

vpsf:	vpsf.c
	${CC} -o vpsf ${CFLAGS} vpsf.c

vdmp: vdmp.c
	${CC} -o vdmp ${CFLAGS} vdmp.c

vpltdmp: vpltdmp.c
	${CC} -o vpltdmp ${CFLAGS} vpltdmp.c -lpixrect

vplotf: vplotf.c chrtab.o
	${CC} -o vplotf ${CFLAGS} vplotf.c chrtab.o

vfontinfo: vfontinfo.c
	${CC} -o vfontinfo ${CFLAGS} vfontinfo.c

vwidth: vwidth.c
	${CC} -o vwidth ${CFLAGS} vwidth.c

vfw: vfw.c
	${CC} -o vfw ${CFLAGS} vfw.c

fcvt: fcvt.c
	${CC} -o fcvt ${CFLAGS} fcvt.c

rotate: rotate.c
	${CC} -o rotate ${CFLAGS} rotate.c

rotprt: rotprt.c
	${CC} -o rotprt ${CFLAGS} rotprt.c

vswap: vswap.c
	${CC} -o vswap ${CFLAGS} vswap.c

install: installp installv

installp: printer
	for i in ${PRINTER}; do \
		install -s $$i ${DESTDIR}${LIBDIR}; \
	done

installv: varian
	install -s vfontinfo ${DESTDIR}${LIBDIR}/vfontinfo
	install -s vwidth ${DESTDIR}${LIBDIR}/vwidth
	install -s vfw ${DESTDIR}${LIBDIR}/vfw
	install -s fcvt ${DESTDIR}${LIBDIR}/fcvt
	install -s rotate ${DESTDIR}${LIBDIR}/rotate
	install -s rotprt ${DESTDIR}${LIBDIR}/rotprt
	install -s rvsort ${DESTDIR}${LIBDIR}/rvsort
	install -s rvcat ${DESTDIR}${LIBDIR}/rvcat
	install -s vsort ${DESTDIR}${LIBDIR}/vsort
	install -s vcat ${DESTDIR}${LIBDIR}/vcat
	install -s vpf ${DESTDIR}${LIBDIR}/vpf
	rm -f ${DESTDIR}${LIBDIR}/vpfW
	ln ${DESTDIR}${LIBDIR}/vpf ${DESTDIR}${LIBDIR}/vpfW
	install -s vpsf ${DESTDIR}${LIBDIR}/vpsf
	rm -f ${DESTDIR}${LIBDIR}/vpsfW
	ln ${DESTDIR}${LIBDIR}/vpsf ${DESTDIR}${LIBDIR}/vpsfW
	install -s vdmp ${DESTDIR}${LIBDIR}/vdmp
	install -s vpltdmp ${DESTDIR}${LIBDIR}/vpltdmp
	install -s vplotf ${DESTDIR}${LIBDIR}/vplotf

clean:
	rm -f ${PRINTER} ${VARIAN} *.o

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c
