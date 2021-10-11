#
# @(#)Makefile 1.1 92/07/30 SMI
#

# makefile for clear_colormap, screendump, screenload, and misc.
# rasterfile utilities and filters

DESTDIR=
BINDIR= /usr/bin
FILTERDIR = /usr/lib/rasfilters
# RELBINDIR is BINDIR relative to FILTERDIR
RELBINDIR= ../../bin
OWNER = bin
INSTALLX = install -c -o $(OWNER) 
INSTALL = $(INSTALLX) -m 644
INSTALLD = $(INSTALLX) -d -m 755

MERGE= screenmerge
BINMERGE= screendump
LINKS= screenload rasfilter8to1 rastrepl clear_colormap rasfilter_rgbtobgr
CMDS= $(BINMERGE) $(LINKS)

SRC= screendump.c screenload.c rasfilter8to1.c rastrepl.c clear_colormap.c
OBJ= $(SRC:.c=.o)

UTIL= screenutil.o

FILTSRC= convert.c
FILTERS= convert.0 convert.2

LDLIBS= -lpixrect

# compilation flags
INC= -I../../include -I../../sys
OPTIM= -O
CFLAGS = $(INC) $(OPTIM)
$(OBJ) := CFLAGS += -DMERGE

default: cmds filters

cmds: $(CMDS)
merge: $(MERGE)
filters: $(FILTERS)
all: cmds filters

$(CMDS): $$@.c $(UTIL)
	$(LINK.c) $@.c $(UTIL) $(LDLIBS) -o $@

convert.0 convert.2: convert.c
	$(LINK.c) -DMY_RAS_TYPE=$(@:convert.%=%) convert.c $(LDLIBS) -o $@

$(MERGE): screenmerge.o $(OBJ) $(UTIL)
	$(LINK.c) screenmerge.o $(OBJ) $(UTIL) $(LDLIBS) -o $@

$(UTIL) $(CMDS) $(OBJ): screendump.h

install: install_cmds install_filters

install_cmds: $(CMDS) $(DESTDIR)$(BINDIR)
	@$(RM) $(CMDS:%=$(DESTDIR)$(BINDIR)/%)
	$(INSTALLX) -s $(CMDS) $(DESTDIR)$(BINDIR)

install_filters: $(FILTERS) $(DESTDIR)$(FILTERDIR)
	$(INSTALL) convert.c $(DESTDIR)$(FILTERDIR)/convert.2.c
	$(INSTALL) rasfilter_rgbtobgr.c $(DESTDIR)$(FILTERDIR)/convert.3.c
	$(INSTALLX) -s convert.0 $(DESTDIR)$(FILTERDIR)
	$(INSTALLX) -s convert.2 $(DESTDIR)$(FILTERDIR)
	$(RM) $(DESTDIR)$(FILTERDIR)/convert.3
	ln -s $(RELBINDIR)/rasfilter_rgbtobgr $(DESTDIR)$(FILTERDIR)/convert.3

DESTLINKS = $(LINKS:%=$(DESTDIR)$(BINDIR)/%)

install_merge: $(MERGE) $(DESTDIR)$(BINDIR) $(DESTLINKS)
	$(INSTALLX) -s $(MERGE) $(DESTDIR)$(BINDIR)/$(BINMERGE)

$(DESTLINKS): FRC
	@$(RM) $@
	ln -s $(MERGE) $@

$(DESTDIR)$(BINDIR) $(DESTDIR)$(FILTERDIR):
	$(INSTALLD) $@

clean: FRC
	$(RM) $(CMDS) $(FILTERS) $(MERGE) *.o a.out core errs*

FRC:
