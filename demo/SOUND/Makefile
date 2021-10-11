#
# @(#)Makefile 1.1 92/07/30 SMI
#

# NOTE WELL:
#
# This Makefile is used to build the audio demo 'release'.
#
# Makefile.obj is shipped as /usr/demo/SOUND/Makefile
#
# When files are added to this makefile, they must generally be added
# to Makefile.obj, too.
#

.KEEP_STATE:

DESTDIR =
INSDIR = $(DESTDIR)/usr/demo/SOUND

# source files in this directory
PROGSRC = play.c record.c raw2audio.c soundtool.c gaintool.c gaintool_ui.c
PROGSRC_H = gaintool_ui.h
ICONS = soundtool.icon gaintool.icon gaintool.mask.icon

PROGS = $(PROGSRC:%.c=%)
EXECS = soundtool gaintool play record raw2audio

# files that are built elsewhere but get installed from this directory
LIBRARY = libaudio.a
LIBDIR = libaudio
.PRECIOUS: $(LIBRARY) $(LIBDIR)/$(LIBRARY)

# 'make install' targets
INSSRC = $(LIBRARY) $(PROGSRC) $(ICONS) Makefile README cuckoo.clock $(PROGSRC_H)
INSFILES = $(INSDIR) $(INSSRC:%=$(INSDIR)/%)

SUBDIR = multimedia $(LIBDIR) sounds

INCLUDE =

# to build the programs in this directory, use Makefile.obj
all default $(PROGS): $(INSSRC)
	?@$(MAKE) $(MFLAGS) -f Makefile.obj $@ INCLUDE=$(INCLUDE)


# make libraries and programs (to verify they build), then install sources only
install: $(INSFILES)
	$(INSTALL) -m 555 $(EXECS) $(INSDIR)
	@for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $@ INCLUDE=$(INCLUDE) \
		DESTDIR=$(DESTDIR) OWNER=$(OWNER));\
	done


# audio library is built in a subdirectory and copied to here
$(LIBRARY): $(LIBDIR)/$(LIBRARY)
	cp $(LIBDIR)/$(LIBRARY) .
	ranlib $(LIBRARY)

$(LIBDIR)/$(LIBRARY):
	@for i in $(SUBDIR); do \
	    (cd $$i && $(MAKE) $(MFLAGS) INCLUDE=$(INCLUDE)); \
	done


# rules for installing files from this directory
OWNER = bin
INSTALL = install -o $(OWNER)

$(INSDIR)/Makefile: Makefile.obj
	$(INSTALL) -m 644 Makefile.obj $(INSDIR)
	mv $(INSDIR)/Makefile.obj $(INSDIR)/Makefile

$(INSDIR)/%.a: %.a
	$(INSTALL) -m 644 $< $(INSDIR)
	ranlib $@

$(INSDIR)/%: %
	$(INSTALL) -m 644 $< $(INSDIR)

# rule to ensure target directories exist
$(INSDIR):
	?$(INSTALL) -d -m 755 $@


clean:
	$(RM) $(LIBRARY)
	@$(MAKE) $(MFLAGS) -f Makefile.obj $@

FRC:
