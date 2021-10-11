#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 5/30/85
#
DESTDIR=
CFLAGS=-O 
OBJS= pow.o gcd.o msqrt.o mdiv.o mout.o mult.o madd.o util.o
SRCS= pow.c gcd.c msqrt.c mdiv.c mout.c mult.c madd.c util.c
TAGSFILE= tags

libmp.a: $(OBJS)
	ar cr libmp.a $(OBJS)

install: libmp.a
	install -m 644 libmp.a ${DESTDIR}/usr/lib/libmp.a
	ranlib ${DESTDIR}/usr/lib/libmp.a

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} libmp.a tags

lint:
	@lint -bhx $(COPTS) $(SRCS) | \
		grep -v "possible pointer alignment problem" 
