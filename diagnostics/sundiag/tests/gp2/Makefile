#
#       @(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

ALL=	gp2test
DESTDIR=
INCS=gp2test.h gp2_hardware.h gp2_matrix.h gp2_polygon.h gp2test_msgs.h 
SLIBS   = -lsuntool -lsunwindow -lpixrect

DBX=
# specify DBX=-g for dbx version

CFLAGS= $(DBX) -fsingle -I${INCDIR} -D`arch`

all:	${INCS} ${ALL}

gp2test: gp2test.o gp2_probe.o gp2_hardware.o gp2_matrix.o \
	gp2_point.o gp2_polygon.o gp2test_msgs.o gp2gpcitest.o \
	crane_init.o gp2_arbiter.o 
	${CC} -o gp2test gp2test.o gp2_probe.o gp2gpcitest.o \
	gp2_hardware.o gp2_matrix.o gp2_point.o gp2_polygon.o gp2test_msgs.o \
	crane_init.o gp2_arbiter.o \
	-lm ${SDLIBS} ${SLIBS}

gp2test.o: gp2test.c gp2test.h
	${CC} -c ${CFLAGS} gp2test.c

gp2_probe.o: gp2_probe.c
	${CC} -c ${CFLAGS} gp2_probe.c

gp2gpcitest.o: gp2gpcitest.c
	${CC} -c ${CFLAGS} gp2gpcitest.c

gp2_hardware.o: gp2_hardware.c gp2_hardware.h
	${CC} -c ${CFLAGS} gp2_hardware.c

gp2_matrix.o: gp2_matrix.c gp2_matrix.h
	${CC} -c ${CFLAGS} gp2_matrix.c

gp2_point.o: gp2_point.c
	${CC} -c ${CFLAGS} gp2_point.c

gp2_polygon.o: gp2_polygon.c gp2_polygon.h
	${CC} -c ${CFLAGS} gp2_polygon.c

gp2_msgs.o: gp2_msgs.c gp2_msgs.h
	${CC} -c ${CFLAGS} gp2_msgs.c

crane_init.o: crane_init.c 
	${CC} -c ${CFLAGS} crane_init.c
gp2_arbiter.o:	gp2_arbiter.c gp2_polygon.h
	${CC} -c ${CFLAGS} gp2_arbiter.c

install: all
	@if [ $(DESTDIR) ]; then \
	  set -x; install -s ${ALL} ${DESTDIR}; \
	else \
	  set -x; install $(ALL) ../../bin; \
	fi

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?
include ../../include/Makefile.end
