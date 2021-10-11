#
# @(#)Makefile 1.1 92/07/30 SMI
#
#
# Makefile to build XGL demo programs. You must define the environment
# variables OPENWINHOME and XGLHOME before making the programs.
#
#
# If you are on a Sun3, you may want to use a version of the XGL library
# this is compiled for a specific floating point processor; change
# the XGL_LIB and FP_TYPE macros.

DESTDIR =
OPENWINHOME= /usr/openwin

INSDIR = $(DESTDIR)/usr/demo/XGL/SRCS
OWNER = bin
INSTALLX = install -o $(OWNER) 
INSTALL = $(INSTALLX) -m 644
INSTALLD = $(INSTALLX) -d -m 755
INSTALLH = $(INSTALLX) -m 444
INSTALLXGL = $(INSTALLX) -m 555
INSTALLDATA = $(INSTALLX) -m 444

DATA = XGL.icon sphere_i.tri
INCL  = P_include.h WS_macros.h

SUBDIR = 

.KEEP_STATE:

MAKEFILE =	Makefile
README = 	README

XGL_DEMOS =	mandelbrot test_sphere xgl_logo
OTHER_DEMOS =	
DEMOS =		$(XGL_DEMOS) $(OTHER_DEMOS)

DEMOSRC =	$(DEMOS:%=%.c)
MISCSRC =	$(README) $(MAKEFILE) $(DATA) $(INCL)
ALLSRC =	$(MISCSRC) $(DEMOSRC)

CPPFLAGS = -I$(OPENWINHOME)/share/include -I$(XGLHOME)/include

# Define this to be -ffpa or -f68881 for a Sun3.
FLOAT-sun2 =	-fswitch
FLOAT-sun3 =	-fswitch
FLOAT-sun4 =
FLOATFLAG =	-fsingle $(FLOAT$(TARGET_ARCH))
CFLAGS = $(CPPFLAGS) -O $(FLOATFLAG)

OW_LIBS =  -lxview -lolgx -lX11 
# flags passed to inferior make
PASSFLAGS=  DESTDIR="$(DESTDIR)" OWNER="$(OWNER)" CFLAGS="$(CFLAGS)"


# change to -lxgl68881 or -lxglfpa.a for Sun3 floating point processors
XGL_LIB = -lxgl

LIBS = -L$(OPENWINHOME)/lib -L$(XGLHOME)/lib $(XGL_LIB) $(OW_LIBS) -lm

UT = quat_utils.o view_3d_set.o

#
#all default: $(ALLSRC) $(DEMOS) subdirs
all default: $(ALLSRC) $(DEMOS) 

$(DEMOS): $(UT) $$@.o
	$(LINK.c) $@.o $(UT) $(LIBS) -o $@

subdirs: FRC
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $(PASSFLAGS)); \
	done

#$(XGL_DEMOS): $$(@).c
#	$(CC) $(CFLAGS) -o $@ $@.c $(LIBS)


clean: FRC
#
	$(RM) *.o errs* core $(DEMOS)
#	for i in $(SUBDIR); do \
#		(cd $$i && $(MAKE) $(MFLAGS) $@); \
#	done

install: all FRC
	$(INSTALLD) $(INSDIR)
#	$(INSTALL)  $(DEMOSRC) $(INSDIR) 
#	$(INSTALL)  $(MAKEFILE) $(INSDIR) 
	$(INSTALL)  $(README) $(INSDIR) 
	$(INSTALL)  $(DATA) $(INSDIR) 
	$(INSTALL)  $(INCL) $(INSDIR) 
	$(INSTALLXGL)  $(XGL_DEMOS) $(INSDIR)
	$(INSTALLDATA)  $(DATA) $(INSDIR)

#	for i in $(SUBDIR); do \
#		$(INSTALLD) $(INSDIR)/$$i; \
#		(cd $$i && $(MAKE) $(MFLAGS) $@ $(PASSFLAGS)); \
#	done

FRC:

