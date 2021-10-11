#
#	@(#)Makefile 1.1 92/07/30 SMI;
#
.DEFAULT:
	sccs get -G$@ $@

OS=
#specify "OS=" for 4.x builds

WIN=
#specify "WIN=" for sunview version of sundiag

REV=NEW
##### Some code will compile only on new revs of the 4.0 SunOS. specify "REV="
##### if built on older versions(before 4.0.3).

DESTDIR=
##### DESTDIR needs to be specified as an absolute directory #####

SUNDIAGDIR=usr/diag/sundiag
##### to be appended after $(DESTDIR) #####

DCPBINARY= dcptest
##### DCP releated binary, to be removed for export.

# Programs that live in subdirectories, and have makefiles of their own.
SUBDIR= include lib probe sunview tests 

# Directories that must exist before make install
DIRS= $(DESTDIR)/$(SUNDIAGDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:$(SUBDIR)

$(SUBDIR): FRC
	@(set -x; cd $@; make $(MFLAGS) all REV=$(REV) OS=$(OS) WIN=$(WIN)) 

install: get_ref_file what_rev FRC
	@if [ $(DESTDIR) ]; then \
	  for i in $(DIRS); do \
	    set -x; rm -rf $$i; mkdir -p $$i; \
	  done; \
	  for i in $(SUBDIR); do \
	    set -x; \
	    (cd $$i; make $(MFLAGS) install OS=$(OS) \
		DESTDIR=$(DESTDIR)/$(SUNDIAGDIR) REV=$(REV) WIN=$(WIN)); \
	  done; \
	  chmod +x what_rev; \
	  cp what_rev $(DESTDIR)/$(SUNDIAGDIR); \
	  chmod +x get_ref_file; \
	  get_ref_file -i $(DESTDIR)/$(SUNDIAGDIR) \
		 -o $(DESTDIR)/$(SUNDIAGDIR); \
	else \
	  set -x; rm -rf bin; mkdir bin; \
	  for i in $(SUBDIR); do \
	    set -x; \
	    (cd $$i; make $(MFLAGS) install REV=$(REV) OS=$(OS) WIN=$(WIN)) ; \
	  done; \
	  chmod +x what_rev; \
	  cp what_rev bin; \
	  chmod +x get_ref_file; \
	  get_ref_file -i bin -o bin; \
	fi

international:
	rm -f $(DESTDIR)/$(SUNDIAGDIR)/($DCPBINARY)

clean: FRC
	@for i in $(SUBDIR); do (set -x; cd $$i; make $(MFLAGS) clean); done

lint: FRC
	@for i in $(SUBDIR); do (set -x; cd $$i; \
				 make $(MFLAGS) lint REV=$(REV)); done

info: FRC
	sccs info
	@for i in $(SUBDIR); do (set -x; cd $$i; make $(MFLAGS) info); done
FRC:
