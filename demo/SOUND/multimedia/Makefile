#
# @(#)Makefile 1.1 92/07/30 SMI
#

# NOTE WELL:
#
# When files are added to this makefile, they must generally be added
# to ../Makefile.obj, too.
#

DESTDIR =
INSDIR = $(DESTDIR)/usr/demo/SOUND/multimedia

# source files in this directory
HDRS = libaudio.h audio_device.h audio_errno.h audio_filehdr.h audio_hdr.h \
	archdep.h ulaw2linear.h

# 'make install' targets
INSFILES = $(INSDIR) $(HDRS:%=$(INSDIR)/%)

# default rule only needs to ensure that the headers are pulled out from SCCS
all default: $(HDRS)

# install header files
install install_h: $(INSFILES)


# rules for installing files from this directory
OWNER = bin
INSTALL = install -o $(OWNER)

$(INSDIR)/%: %
	$(INSTALL) -m 644 $< $(INSDIR)

# rule to ensure target directories exist
$(INSDIR):
	?$(INSTALL) -d -m 755 $@


clean:
