#
# @(#)Makefile 1.1 92/07/30 SMI
#

# Makefile for SunCore library compilation

SCRIPT= install_from_old

SRC=	auxiliary.c batch.c bw1subs.c bwdd.c bwddstub.c \
	cg1subs.c cgdd.c cgddstub.c cgpixwindd.c \
	cgpixwinsubs.c cmapshared.c credelseg.c devecho.c errors.c \
	get_view_surface.c gp1_attr.c gp1_control.c gp1_prims.c \
	gp1_segdraw.c gp1dd.c gp1pixwindd.c gp1subs.c init_termin.c \
	inprims.c inputctrl.c inputdds.c inqinput.c inqprimatt.c \
	inqsegatt.c inqtextent.c inquiry.c matinv.c ndctowld.c \
	newframe.c outprim2.c outprim3.c outputclip.c pdfio.c \
	pixwindd.c pixwinsubs.c polygon2.c polygon3.c rasterfileio.c \
	rasterprim.c rawddsubs.c repaint.c round.c savesegment.c \
	scribe.c segatt3.c segdefaults.c segdraw.c segments.c \
	setmatrix.c setprimatt.c setsegatt.c sfont.c sqrttable.c \
	textmark.c view_surface.c view_trans.c windowddsubs.c \
	winsig.c winzbuffer.c xformattrib.c xformclip2.c \
	xformclip3.c zbuffer.c

LOCAL_HDR = bw2dd.h cgdd.h cgpixwindd.h colorshader.h coretypes.h \
	corevars.h corewinvars.h gp1_pwpr.h gp1dd.h \
	gp1pixwindd.h langnames.h pixwindd.h usercore.h

HDR= usercore.h

HDRDIR= $(DESTDIR)/usr/old
INSDIR= $(DESTDIR)/usr/old

LIBHDRDIR= $(DESTDIR)/usr/include
LIBINSDIR= $(DESTDIR)/usr/lib

BASEOBJ= $(SRC:.c=.o)

OBJDIR= Obj-norm
OBJ= $(BASEOBJ:%=$(OBJDIR)/%)
OBJLIB= $(OBJDIR)/libcore.a

SHOBJDIR= Obj-shared
SASTUB= sastub
SASRC=	_cgpixwindd.c _gp1dd.c _gp1pixwindd.c _pixwindd.c 
SAOBJ1= bwddstub.o cgddstub.o $(SASRC:.c=.o)
SAOBJ= $(SAOBJ1:%=$(SHOBJDIR)/%)
SHSRC= $(SRC) $(SASRC)
SHOBJ= $(SHSRC:%.c=$(SHOBJDIR)/%.o)
SHOBJLIB= $(SHOBJDIR)/libcore.so
SALIB= $(SHOBJDIR)/libcore.sa

# floating point variants
# XXX wish I could think of a better way to do this

FOPT-sun2= sky
FOPT-sun3= 68881 fpa
FOPT= $(FOPT$(TARGET_ARCH)
ALLARCH= sun2 sun3
ALLFOPT= sky 68881 fpa

ALLFOBJDIR= $(ALLFOPT:%=Obj-%)

FOBJLIB-sun2= Obj-sky/libcoresky.a
FOBJLIB-sun3= Obj-68881/libcore68881.a Obj-fpa/libcorefpa.a
FOBJLIB= $(FOBJLIB$(TARGET_ARCH))
ALLFOBJLIB= $(FOBJLIB-sun2) $(FOBJLIB-sun3)

FOBJSKY= $(BASEOBJ:%=Obj-sky/%)
FOBJ68881= $(BASEOBJ:%=Obj-68881/%)
FOBJFPA= $(BASEOBJ:%=Obj-fpa/%)
ALLFOBJ= $(FOBJSKY) $(FOBJ68881) $(FOBJFPA)

LIB= $(OBJLIB) $(FOBJLIB) $(SHOBJLIB) $(SALIB)

# compile flags
FLOAT-sun2= -fsoft
FLOAT-sky= -fsky
FLOAT-sun3= -fsoft
FLOAT-68881= -f68881 /usr/lib/f68881/libm.il
FLOAT-fpa= -ffpa /usr/lib/ffpa/libm.il
FLOAT-sun4= /usr/lib/libm.il
FLOATOPT= $(FLOAT$(TARGET_ARCH))
FLOAT= -fsingle $(FLOATOPT)
INC= -I.
OPTIM= -O
CFLAGS= $(FLOAT) $(INC) $(OPTIM)

# shared lib stuff
SHFLAGS= -pic -DPIC -DSHLIB
SHLD= $(LD) -assert pure-text
SHVER= 0.1

ARFLAGS= rc
OWNER= bin
INSTALL= install -o $(OWNER)
CTAGS= ctags -t -w

.KEEP_STATE:

.INIT:  $(LOCAL_HDR) $(SCRIPT)

all: $(LIB)

$(OBJLIB) $(ALLFOBJLIB): $(BASEOBJ:%=$$(@D)/%)
	cd $(@D) && $(AR) $(ARFLAGS) $(@F) $(?:$(@D)/%=%)
	ranlib $@

$(SHOBJLIB): $(SHOBJ)
	cd $(@D) && $(SHLD) -o $(@F) $(SHOBJ:$(@D)/%=%)

$(OBJ) $(ALLFOBJ) $(SHOBJ): $$(@D) $$(@F:.o=.c)
	$(COMPILE.c) -o $@ $(@F:.o=.c)

$(ALLFOBJ) := FLOATOPT= $(FLOAT-$(@D:Obj-%=%))

$(SALIB): $(SAOBJ)
	cd $(@D) && $(AR) $(ARFLAGS) $(@F) $(?:$(@D)/%=%)
	ranlib $@

$(SASRC): $(SASTUB)
	sh $(SASTUB) $(@F:_%.c=%) > $@

$(SHOBJ) := CFLAGS += $(SHFLAGS)

$(OBJDIR) $(ALLFOBJDIR) $(SHOBJDIR):
	-mkdir $@


# default is to install in /usr/old
install: install_old

install_lib install_old: install_h install_libs
install_old: install_script

install_lib := HDRDIR = $(LIBHDRDIR)
install_lib := INSDIR = $(LIBINSDIR)

install_h: $(HDR)
	$(INSTALL) -d -m 755 $(HDRDIR)
	$(INSTALL) -m 444 $(HDR) $(HDRDIR)

install_libs: $(LIB)
	$(INSTALL) -d -m 755 $(INSDIR)
	$(INSTALL) -m 644 $(OBJLIB) $(FOBJLIB) $(INSDIR)
	for i in $(OBJLIB) $(FOBJLIB) ; do \
		ranlib $(INSDIR)/`basename $$i`; \
	done
	$(INSTALL) -m 755 $(SHOBJLIB) $(INSDIR)/libcore.so.$(SHVER)
	$(INSTALL) -m 755 $(SALIB) $(INSDIR)/libcore.sa.$(SHVER)
	ranlib $(INSDIR)/libcore.sa.$(SHVER)

install_script: $(SCRIPT)
	$(INSTALL) -d -m 755 $(INSDIR)
	$(INSTALL) -m 755 $(SCRIPT) $(INSDIR)	

clean: FRC
	$(RM) $(SASRC) tags *.o a.out core errs*
	$(RM) -r $(OBJDIR) $(ALLFOBJDIR) $(SHOBJDIR)

lint: $(SRC)
	$(LINT.c) $(SRC)

tags: $(LOCAL_HDR) $(SRC)
	$(CTAGS) $(LOCAL_HDR) $(SRC)

FRC:
