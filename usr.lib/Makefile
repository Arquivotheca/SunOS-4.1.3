#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.10 83/07/09
#

DESTDIR=
INSDIR= $(DESTDIR)/usr/lib
INSDIRS5 = $(DESTDIR)/usr/5lib

CCS5= /usr/5bin/cc
CFLAGS=	-O

OWNER= bin
INSTALLX= install -o $(OWNER)
INSTALLD= $(INSTALLX) -d -m 755
INSTALL= $(INSTALLX) -m 644

# Libraries and programs that live in subdirectories, that should be built
# with the 4.2 C compiler, and that have makefiles of their own.
#
SUBDIR= librpcsvc libplot libdbm libm \
	libln libmp locale \
	me ms tmac vfont fixedwidthfonts \
	libcore libcore77 libcorepas libg \
	libpixrect libsuntool libsunwindow libcgi libcgi77\
	liblwp libresolv libkvm libdl sendmail

# Libraries and programs that live in subdirectories, that should be built
# with the S5 C compiler, and that have makefiles of their own.
#
S5SUBDIR= acct libcurses liby terminfo libnsl libns libsvidm

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	makewhatis newsyslog
SCRIPTSH=	makewhatis.sh newsyslog.sh
SCRIPTSRC=	makewhatis newsyslog

# Text files that need only be installed and are never removed.

FILES= Cshrc Exrc Login Logout Mailrc
LIBFILE= lib.b

# C programs that live in the current directory, that should be built
# with the 4.2 C compiler, and that do not need explicit make lines.
#
STD=	getNAME makekey

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	

all:	$(STD) $(SUBDIR) $(S5SUBDIR) gprof

$(SUBDIR): FRC
	cd $@ && $(MAKE) $(MFLAGS)

$(S5SUBDIR): FRC
	cd $@ && $(MAKE) CC=$(CCS5) $(MFLAGS)

gprof: FRC
	for i in libsunwindow libsuntool; do \
		(cd $$i && $(MAKE) $(MFLAGS) $@) \
	done

install: $(SCRIPTSH) $(LIBFILE) $(STD) $(SCRIPTSRC) $(FILES) FRC
	@set -x;for i in $(INSDIRS5) $(INSDIR) $(INSDIR)/fonts/fixedwidthfonts;\
		do $(INSTALLD) $$i; done
	$(INSTALLX) $(SCRIPT) $(INSDIR)
	$(INSTALL) $(FILES) $(LIBFILE) $(INSDIR)
	$(INSTALLX) -s $(STD) $(INSDIR)
	@set -x; for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) DESTDIR=$(DESTDIR) $@); done
	@set -x; for i in $(S5SUBDIR); do \
		(cd $$i && $(MAKE) CC=$(CCS5) $(MFLAGS) DESTDIR=$(DESTDIR) $@);\
		done

clean: FRC
	$(RM) a.out core *.s *.o $(STD)
	set -x; for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $@); done
	set -x; for i in $(S5SUBDIR); do \
		(cd $$i && $(MAKE) CC=$(CCS5) $(MFLAGS) $@); done

FRC:

