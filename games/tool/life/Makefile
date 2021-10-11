#
# @(#)Makefile 1.1 92/07/30 SMI
#

OBJECTS = lifetool.o life.o circle.o patterns.o prompt.o
NO_MAIN_OBJS =	life.o circle.o patterns.o prompt.o

LIBS =  -lsuntool -lsunwindow -lpixrect -lm

CFLAGS = -O
CC = /bin/cc

life:	$(OBJECTS)
	$(CC) -o life $(CFLAGS) $(OBJECTS) $(LIBS)

standalone: ${NO_MAIN_OBJS}
	$(CC) -DSTANDALONE ${CFLAGS} -o life lifetool.c \
	${NO_MAIN_OBJS} ${LIBS}

lifetool.o: life.h
life.o: life.h
patterns.o: life.h
prompt.o: ml.icon
prompt.o: mr.icon

clean: 
	-rm $(OBJECTS) core life

install: life
	install -s -o daemon life ${DESTDIR}/usr/games/life

