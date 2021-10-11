# Makefile for memtest.o (loadable device driver)
#
ARCH=	sun4c
CPP=	/lib/cpp -undef -Dsparc -Dsun
CC=	cc
AS=	as
LD=	ld
AWK=	awk
GREP=	egrep
SYSDIR=	../..
MACHINE= ..

IDENT=-Dsun4c -DGENERIC -DSUN4C_60 -DVDDRV -DCRYPT -DTCPDEBUG -DIPCSHMEM -DIPCSEMAPHORE -DIPCMESSAGE -DSYSACCT -DLOFS -DNFSSERVER -DNFSCLIENT -DUFS -DQUOTA -DINET -DSAIO_COMPAT -DMEMSEG -DOPENPROMS -DFIXED_PROMS -DP1_5 -DPAGE_DEBUG -DSCSI

CPPOPTS=${IDENT} -DKERNEL -I. -I${MACHINE} -I${SYSDIR}
COPTS=	${CPPOPTS}
CFLAGS=	-O ${COPTS}

OBJS = memtest.o

SCRIPTS = memtest_load memtest_unload

all: ${OBJS} ${SCRIPTS} testprog

memtest.o: memtest.c memtestio.h
	${CC} -c ${CFLAGS} memtest.c

load: memtest.o memtest_load
	modload -sym memtest.o -entry _memtest_init -exec memtest_load

unload: memtest_unload memtestID
	modunload -id `cat memtestID` -exec memtest_unload

clean:
	-rm -f *.o

testprog: test_memtest

test_memtest: test_memtest.o test_memtest_asm.o
	${CC} -I. -O -o test_memtest -Bstatic test_memtest.o test_memtest_asm.o

test_memtest.o: test_memtest.c memtestio.h
	${CC} -I. -O -c test_memtest.c

test_memtest_asm.o: test_memtest_asm.S
	${CC} -I. -O -c test_memtest_asm.S

install: all /Memtest
	cp Makefile *memtest* /Memtest
	-(cd /Memtest; chmod +w *)
	date > /Memtest/Timestamp

/Memtest:
	mkdir /Memtest

update:
	(cd /Memtest; find . -newer Timestamp -type f -print) | \
		awk '{printf "cp %s .\n", $$1}' | sh -x
