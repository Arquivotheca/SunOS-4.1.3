#
# @(#)Makefile 1.1 92/07/30 SMI
#

# Makefile for Egret System Diagnostics

.DEFAULT:
	$(SCCS) get $(@F)

include ../../include/Makefile.macros
REV=
# specify "REV=" if built for 4.0.X

DBX=-g
# specify DBX=-g for dbx version

SUNDIAG_DIR=	/usr/diag/sundiag

TEST_IMAGES=	cg12.data cg12.data.gsxr
TARGETS=	cg12 $(TEST_IMAGES)

#sources & objectS
SRCS=		cg12.c\
		sb_testlist.c\
		timestamp.c\
		catch_signals.c\
		getfname.c\
		pr_test.c\
		desktop.c\
		chksum.c\
		msg.c\
		xf.c\
		lines.c\
		clip_hid.c\
		dsp.c\
		poligons.c\
		sram_dram.c\
		shad.c\
		video_mem.c\
		lut.c\
		animation.c\
		depth_cueing.c\
		gpsi_3D_lines.c\
		gpsi_3D_poligons.c\
		gpsi_animation.c\
		gpsi_clip_hid.c\
		gpsi_depth_cueing.c\
		gpsi_shad.c\
		gpsi_uti.c\
		gpsi_xf.c\
		patterns.c\
		do_animation.c\
		shaded_cylinder.c\
		textured_hexnut.c\
		disks.c\
		picktest.c

OBJS=	$(SRCS:.c=.o)


# commands
SCCS=		/usr/ucb/sccs
RM=		/bin/rm -f
ED=		/bin/ed
CP=		/bin/cp
ED=		/bin/ed
CPP=		/lib/cpp -E
LINT=		lint

# system include directory
STDINCL= /usr/include

# used during development to include new or modified include files
DEVEL_INCLPATH=

# used during development to include new or modified include files
DEVEL_LIBPATH=

# sundiag test lib
SUNDIAG_LIBS= -L$(TESTDIR) -L$(ONLINEDIR)

# compilation flags
FLOAT= -fsoft
DEFINES=
INCLUDES= -I. -I$(INCDIR)

CFLAGS= $(FLOAT) $(DBX)
CPPFLAGS = $(INCLUDES) $(DEFINES)
CPPFLAGS_DEP = $(INCLUDES) -I$(INCDIR) -I$(ONLINEINC) $(DEFINES)
LDFLAGS= $(SUNDIAG_LIBS) $(DEBUG)

LINT_FLAGS= -abh $(INCLUDES) -I$(INCDIR) -I$(ONLINEINC) $(DEFINES)

LIBS= -ltest -lonline -lsunwindow -lpixrect -lm

LINT_LIB_PATH= /usr/lib/lint
LINT_LIBS= 
	   
all: $(TARGETS)

cg12:	$(OBJS)
		$(CC) $(LDFLAGS) $(OBJS) -o cg12 $(LIBS)

install: all FRC
	@if [ $(DESTDIR) ]; then \
	    set -x; \
	    install -s $(TARGETS) $(DESTDIR); \
	    install $(TEST_IMAGES) $(DESTDIR); \
	else \
	    set -x; \
	    install $(TARGETS) ../../bin; \
	    install $(TEST_IMAGES) ../../bin; \
	fi

lint:	$(SRCS) $(LINT_LIBS)
	$(LINT) $(LINT_FLAGS) $(SRCS) $(LINT_LIBS)

clean:
	$(RM) *.o *.pp *.ln $(TARGETS) core *~ #*

info: FRC
	$(SCCS) info

depend: $(SRCS)
	$(RM) makedep eddep
	for i in $(SRCS) ; do \
	    ( $(CPP) -M $(CPPFLAGS_DEP) $$i >> makedep ); \
	    echo `basename $$i .c`.h: $$i >> makedep ; \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	$(CP) Makefile Makefile.bak
	$(ED) - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	$(RM) eddep makedep

cg12.o: cg12.c esd.h cg12.h $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
	$(CC) $(INCLUDES) -I$(INCDIR) -I$(ONLINEINC) $(CFLAGS) -c cg12.c

FRC:

include ../../include/Makefile.end

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE

