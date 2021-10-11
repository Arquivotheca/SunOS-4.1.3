# @(#)Makefile 1.1 92/07/30 SMI

#
#
#	this is the default definition for OPENWINHOME.  Use setenv and make -e to
#	override it

OPENWINHOME = /usr/openwin/share
DESTDIR= 
INSDIR= $(DESTDIR)/usr/demo/XGL
SUBDIR= SRCS EXECS
OWNER= bin
INSTALL= install -o $(OWNER)

MAKEFILE= Makefile
README= README

all: $(MAKEFILE) $(README) subdirs

subdirs: FRC
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS)); \
	done

clean: FRC
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done

install: $(MAKEFILE) $(README) FRC
	install -d -m 755 $(INSDIR)
#	install -m 644 $(MAKEFILE) $(INSDIR)
	install -m 644 $(README) $(INSDIR)
	for i in $(SUBDIR); do \
		install -d -o $(OWNER) -m 755 $(INSDIR)/$$i && \
		(cd $$i && $(MAKE) $(MFLAGS) $@ \
			DESTDIR=$(DESTDIR) OWNER=$(OWNER)); \
	done

FRC:
