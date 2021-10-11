#
# @(#)Makefile	1.1 8/6/90 SMI
#

include ../Makefile.master

PROG= eeprog
OBJS= eeprog.o

CFLAGS += -DMACHINE=${MACH}
all: eeprog.${MACHNAME}

eeprog.${MACHNAME}: clean eeprog.o
	${CC} -o $(PROG) $(OBJS)
	$(PROG) xx
	mv xx ../${MACHNAME}/eeprom.${MACHNAME}

clean: FRC
	-rm -rf ${OBJS} ${PROG} '#*' *~

FRC:
