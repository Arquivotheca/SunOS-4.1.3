#	Makefile for Sundiag tests.
#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

OS=SVR4
#specify "OS=" for 4.x builds

REV=NEW
##### Some code will compile only on new revs of the SunOS. specify "REV="
##### if built on older versions.

WIN=OL
# specify openlook version of tests for some graphic related ones.

DESTDIR=

# Programs that live in subdirectories, and apply to all architectures
SUBDIR= color disk net pmem pp serial tape vmem newtest

# Programs that live in subdirectories, and not available on SunX86
SUBSUN= c24 cg5 cg6 cg12 fp gp1 gp2 ipc mono parallel presto\
	sunbuttons sundials sunlink taac audio cdrom zebra spif vfc mp gt\
	dbri

# Programs that live in subdirectories, and apply to NEW SunOS only
NEWDIR= 

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(SUBDIR) $(SUBSUN) 

$(SUBDIR): FRC
	@(set -x; cd $@; make $(MFLAGS) all REV=$(REV) OS=$(OS) WIN=$(WIN))

$(SUBSUN): FRC
	@if [ `arch` != "sun386" ] && [ `arch` != "sun486" ]; then \
	   (set -x; cd $@; make $(MFLAGS) all REV=$(REV) OS=$(OS) WIN=$(WIN)); \
	fi

$(NEWDIR): FRC
	@if [ "$(REV)" = "NEW" ]; then \
	   (set -x; cd $@; make $(MFLAGS) all REV=$(REV) OS=$(OS) WIN=$(WIN)); \
	fi

install: FRC
	@for i in $(SUBDIR); \
		do (set -x; cd $$i; make $(MFLAGS) install \
		DESTDIR=$(DESTDIR) REV=$(REV) OS=$(OS) WIN=$(WIN)); done
	@if [ `arch` != "sun386" ] && [ `arch` != "sun486" ]; then \
	  for i in $(SUBSUN); \
		do (set -x; cd $$i; make $(MFLAGS) install \
		DESTDIR=$(DESTDIR) REV=$(REV) OS=$(OS) WIN=$(WIN)); done; \
	fi
#	@if [ "$(REV)" = "NEW" ]; then \
#	  for i in $(NEWDIR); \
#		do (set -x; cd $$i; make $(MFLAGS) install \
#		DESTDIR=$(DESTDIR) REV=$(REV) WIN=$(WIN)); done; \
#	fi

clean: FRC
	@for i in $(SUBDIR); do (set -x; cd $$i; make $(MFLAGS) clean); done
	@if [ `arch` != "sun386" ] && [ `arch` != "sun486" ]; then \
	 for i in $(SUBSUN); do (set -x;cd $$i;make $(MFLAGS) clean); done; \
	fi


lint: FRC
	@for i in $(SUBDIR); \
		do (set -x; cd $$i; make $(MFLAGS) lint REV=$(REV) OS=$(OS) WIN=$(WIN)); done
	@if [ `arch` != "sun386" ] && [ `arch` != "sun486" ]; then \
	   for i in $(SUBSUN); \
		do (set -x; cd $$i; make $(MFLAGS) lint REV=$(REV) OS=$(OS) WIN=$(WIN)); done; \
	fi

info: FRC
	sccs info
	@for i in $(SUBDIR); do (set -x; cd $$i; sccs info); done
	@for i in $(SUBSUN); do (set -x; cd $$i; sccs info); done

FRC:
