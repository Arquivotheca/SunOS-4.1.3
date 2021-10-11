#
# @(#)Makefile	1.1 8/6/90 SMI
#

INCLUDE=../sun4m
CFLAGS=-DOBP -DKERNEL -Dsun4c -I$(INCLUDE)
PROGS= disktest treetest
all: $(PROGS)

disktest: disktest.o
	ld -e _start -N -T 4000 -o disktest disktest.o -lc

treetest: treetest.o
	ld -e _start -N -T 4000 -o treetest treetest.o -lc

clean: FRC
	-rm -rf *.o $(PROGS) *~

install: all
	install $(PROGS) $${RUNDIR}

FRC:
