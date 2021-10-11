#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR =

INSDIR = $(DESTDIR)/usr/demo/SUNVIEW/SRCS
OWNER = bin
INSTALLX = install -o $(OWNER) 
INSTALL = $(INSTALLX) -m 644
INSTALLEX = $(INSTALLX) -m 555
INSTALLD = $(INSTALLX) -d -m 755
INSTALLH = $(INSTALLX) -m 444

SUBDIR = DATA 

MAKEFILE =	Makefile
README = 	README

WINDOW_DEMOS =	cframedemo show maze
OTHER_DEMOS =	
DEMOS =		$(WINDOW_DEMOS) $(OTHER_DEMOS)

DEMOSRC =	$(WINDOW_DEMOS:%=%.c)
MISCSRC =	README Makefile
ALLSRC =	$(MISCSRC) $(DEMOSRC)

WINLIBS =	-lsuntool -lsunwindow -lpixrect

WINLIBS =	-lsuntool -lsunwindow -lpixrect

FLOAT-sun2 =	-fswitch
FLOAT-sun3 =	-fswitch
FLOAT-sun4 =
FLOATFLAG =	-fsingle $(FLOAT$(TARGET_ARCH))
CFLAGS =	-O $(FLOATFLAG)

# flags passed to inferior make
PASSFLAGS=  DESTDIR="$(DESTDIR)" OWNER="$(OWNER)" CFLAGS="$(CFLAGS)"


all: $(ALLSRC) $(DEMOS) subdirs

subdirs: FRC
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $(PASSFLAGS)); \
	done

$(WINDOW_DEMOS): $$(@).c
	$(CC) $(CFLAGS) -o $@ $@.c $(WINLIBS)


clean: FRC
	$(RM) *.o errs* core $(DEMOS)
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done

install: all FRC
	$(INSTALLD) $(INSDIR)
	$(INSTALL) $(MAKEFILE) $(README) $(MISCSRC) $(DEMOSRC) $(INSDIR)
	$(INSTALLEX) $(DEMOS) $(INSDIR)
	for i in $(SUBDIR); do \
		$(INSTALLD) $(INSDIR)/$$i; \
		(cd $$i && $(MAKE) $(MFLAGS) $@ $(PASSFLAGS)); \
	done

FRC:
