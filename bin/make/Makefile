#
# @(#)Makefile 1.1 92/07/30 SMI; Remotely from S5R2 1.7
#
.KEEP_STATE:
.FRC:
CC	= /bin/cc

VROOT	= ../../lang/vroot

LOCAL_HDRS=  defs.h $(VROOT)/report.h $(VROOT)/vroot.h
HDRS	= default.mk
HDRDIR	= $(DESTDIR)/usr/include/make

BINS	= make
MAKEDIR	= $(DESTDIR)/usr/bin
MAKESHDIR = $(DESTDIR)/usr/lib

LINK_LIBS	= $(VROOT)/vroot.a

MAKE_SRCS = \
		ar.c \
		doname.c \
		dosys.c \
		files.c \
		globals.c \
		implicit.c \
		macro.c \
		main.c \
		misc.c \
		read.c \
		read2.c \
		rep.c \
		state.c
MAKE_OBJS = $(MAKE_SRCS:.c=.o)
MAKESH_SRCS = makesh.c dosys.c misc.c
MAKESH_OBJS = $(MAKESH_SRCS:.c=.o)
SRCS = $(MAKE_SRCS) makesh.c
OBJS =  $(SRCS:.c=.o)

CPPFLAGS=-I$(VROOT)
COPTS	= -O
CFLAGS	= $(COPTS)

.INIT: $(HDRS) $(LOCAL_HDRS)

bins: $$(BINS)

install: install_bins install_h

parallel:=	BINS += makesh
parallel:=	CPPFLAGS += -DPARALLEL
parallel:=	MAKE_SRCS += parallel.c
parallel:=	LIBS += -lrpcsvc
parallel:	bins

make: $$(MAKE_OBJS) $(LINK_LIBS)
	$(LINK.c) -o $@ $(MAKE_OBJS) $(LINK_LIBS) $(LIBS)

makesh: $$(MAKESH_OBJS) $(LINK_LIBS)
	$(LINK.c) -o $@ $(MAKESH_OBJS) $(LINK_LIBS)

$(VROOT)/vroot.a:
	cd $(@D); $(MAKE) $(@F)

install_bins: $$(BINS)
	install -d -m 755 $(MAKEDIR)
	install -s make $(MAKEDIR)
#	install -d $(MAKESHDIR)
#	install -s makesh $(MAKESHDIR)

install_h: $(HDRS)
	install -m 755 -d $(HDRDIR)
	install -m 444 $(HDRS) $(HDRDIR)

clean:
	$(RM) -r $(LOCAL_BINS) $(BINS) $(LOCAL_LIBS) $(LIBS) $(OBJS)

lint:	make.lint makesh.lint

make.lint: $$(MAKE_SRCS:.c=.ln)
	$(LINT.c) $(MAKE_SRCS:.c=.ln)

LINT = /usr/bin/lint
makesh.lint: $(MAKESH_SRCS:.c=.ln)
	$(LINT.c) $(MAKESH_SRCS:.c=.ln)
