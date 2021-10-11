#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1989 by Sun Microsystems, Inc.

BASEDIR=	../
VIDEODIR=	../../
CFLAGS=		-O -I$(VIDEODIR) -I$(BASEDIR)

LASER_SRC=main.c cycle_create.c button_create.c info_create.c \
	search_etc_create.c cycle_proc.c button_proc.c search_etc_proc.c\
	poll.c

LASER_OBJ=main.o cycle_create.o button_create.o info_create.o \
	search_etc_create.o cycle_proc.o button_proc.o search_etc_proc.o\
	poll.o

HEADERS=$(BASEDIR)/sony_laser.h lasertool.h 

ICONS=lasertool.icon audio.icon forward.icon forward_on.icon \
	reverse.icon reverse_on.icon

LIBS=$(BASEDIR)/sonylaslib.a $(VIDEODIR)/libvideo.a \
	-lsuntool -lsunwindow -lpixrect

DEST_SRC=	$(DESTDIR)/usr/demo/SUNVIDEO/LASERDISC/lasertool
DEST_BIN=	$(DESTDIR)/usr/demo/SUNVIDEO/LASERDISC/lasertool


lasertool : $(LASER_OBJ)  $(BASEDIR)/sonylaslib.a 
	cc -o $@ $(LASER_OBJ) $(LIBS) 

$(LASER_OBJ) : $(HEADERS) $(ICONS)

clean :
	rm -f *.o lasertool core


install:	lasertool
	install -d -m 755 $(DEST_SRC); \
	install -m 444 $(LASER_SRC) $(DEST_SRC); \
	install -m 444 Makefile $(DEST_SRC)/Makefile; \
	install -m 444 lasertool.h $(DEST_SRC); \
	install -m 444 $(ICONS) $(DEST_SRC); \
	install -m 555 lasertool $(DEST_BIN); \
#
