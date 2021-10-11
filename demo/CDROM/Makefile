#
# Makefile for Cdplayer
# @(#)Makefile 1.1 92/07/30 Copyr 1989 Sun Microsystem, Inc.
#
ARCH:sh= arch -k
SHDDIR = usr/demo/CDROM
SUBDIR = 
OPENWINHOME= /usr/openwin

INSTALL = install -c

CFLAGS= -O -D$(ARCH) $(TARGET_ARCH) -DFUNCTION_GLYPHS -I$(OPENWINHOME)/share/include
LTOOL= -lsuntool
LSUNWINDOW= -lsunwindow
LPIXRECT= -lpixrect
LIBS=   ${LTOOL} ${LSUNWINDOW} ${LPIXRECT} -lm
HFILES   = cdrom.h msf.h toc.h player.h prop_ui.h cycle.button \
	eject.button ff.button pause.button play.button rew.button stop.button


OBJS	= player.o msf.o toc.o cdrom.o
CFILES	= player.c msf.c toc.c cdrom.c
IFILES	= player.icon
EXECS 	= cdplayer x_cdplayer

XOBJS	= x_player.o msf.o toc.o cdrom.o x_prop.o x_prop_ui.o
XCFILES	= x_player.c msf.c toc.c cdrom.c x_prop.c x_prop_ui.c
XLIBS = -L$(OPENWINHOME)/lib -lxview -lolgx -lX

MAKEFILE = Makefile

all: $(HFILES) cdplayer x_cdplayer

cdplayer:	$(OBJS) 
	$(LINK.c) $(OBJS) -o $@ ${CFLAGS}  ${LIBS}

x_cdplayer:	 $(XOBJS) $(HFILES)
	$(LINK.c) $(XOBJS) -o $@ ${CFLAGS}  ${XLIBS}


install: ins_dirs ins_file ins_mk  FRC

ins_file:  ins_dirs ${CFILES} ${XCFILES} ${HFILES} $(IFILES) FRC
		@echo "Installing CDROM source files"
		$(INSTALL) -m 644 -o $(OWNER) \
	  		 $(CFILES) $(XCFILES) ${DESTDIR}/${SHDDIR}
		$(INSTALL) -m 555 -o $(OWNER) \
	  		 $(EXECS) ${DESTDIR}/${SHDDIR}
		$(INSTALL) -m 444 -o $(OWNER) \
	  		 $(HFILES) ${DESTDIR}/${SHDDIR}
		$(INSTALL) -m 644 -o $(OWNER) \
	  		 $(IFILES) ${DESTDIR}/${SHDDIR}

ins_mk:    ins_dirs $(MAKEFILE) FRC
		@echo "Installing make file"
		$(INSTALL) -m 644 -o $(OWNER) \
	   		$(MAKEFILE)  ${DESTDIR}/${SHDDIR}
ins_dirs: FRC
	@echo "Checking CDROM install directories"
	install -d -m 755 $(DESTDIR)/usr
	install -d -m 755 $(DESTDIR)/usr/demo
	install -d -m 755 $(DESTDIR)/usr/demo/CDROM

clean:
	rm -f *.o
	rm -f core
	rm -f *.BAK

FRC:

#
#
#	dependencies from cc -M and filtered by grep -v include
#
#

cdrom.o: cdrom.c
cdrom.o: ./cdrom.h
cdrom.o: ./msf.h
msf.o: msf.c
msf.o: ./msf.h
player.o: player.c
player.o: ./msf.h
player.o: ./toc.h
player.o: ./cdrom.h
player.o: ./player.icon
toc.o: toc.c
toc.o: ./msf.h
toc.o: ./toc.h
x_player.o: x_player.c
x_player.o: ./msf.h
x_player.o: ./toc.h
x_player.o: ./cdrom.h
x_player.o: ./player.h
x_player.o: ./prop_ui.h
x_player.o: ./player.icon
x_prop.o: x_prop.c
x_prop.o: ./prop_ui.h
x_prop.o: ./cdrom.h
x_prop.o: ./msf.h
x_prop.o: ./toc.h
x_prop.o: ./cdrom.h
x_prop.o: ./player.h
x_prop_ui.o: x_prop_ui.c
x_prop_ui.o: ./prop_ui.h
