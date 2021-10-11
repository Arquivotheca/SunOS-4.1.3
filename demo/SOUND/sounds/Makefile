#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR =
INSDIR = $(DESTDIR)/usr/demo/SOUND/sounds

# source files in this directory
PHONE_SOUNDS = busy.au dialtone.au fastbusy.au ring.au \
	touchtone.1.au touchtone.2.au touchtone.3.au \
	touchtone.4.au touchtone.5.au touchtone.6.au \
	touchtone.7.au touchtone.8.au touchtone.9.au \
	touchtone.star.au touchtone.0.au touchtone.pound.au

# The following sound effects are copyright (c) 1989 Sound Ideas
# Sun has express permission to ship up to twenty Sound Ideas sound effects.
SOUND_IDEAS = bark.au bong.au bubbles.au chirp.au clink.au \
	computer.au cowbell.au crash.au cuckoo.au doorbell.au \
	drip.au flush.au gong.au laugh.au laughter.au \
	rooster.au spacemusic.au splat.au train.au whistle.au

SOUNDS = $(PHONE_SOUNDS) $(SOUND_IDEAS) sample.au

# 'make install' targets
INSFILES = $(INSDIR) $(SOUNDS:%=$(INSDIR)/%)


# default rule only needs to ensure that the files are pulled out from SCCS
all default: $(SOUNDS)

# install sound files
install: $(INSFILES)


# rules for installing files from this directory
OWNER = bin
INSTALL = install -o $(OWNER)

$(INSDIR)/%: %
	$(INSTALL) -m 644 $< $(INSDIR)

# rule to ensure target directories exist
$(INSDIR):
	?$(INSTALL) -d -m 755 $@


clean:
