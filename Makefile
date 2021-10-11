#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR	=	/proto
SH	=	/bin/sh
ECHO	=	/bin/echo
RM	=	/bin/rm
DATE	=	/bin/date
CPP	=	/lib/cpp
CHOWN	=	/etc/chown
GREP	=	/usr/bin/grep
LS	=	/bin/ls
PSTAT	=	/usr/etc/pstat
# kludges...
RELEASE_SRC=	./release
INSTALL_SRC=	./usr.bin
MAKE_SRC=	./bin/make
SCCS_SRC=	./sccs
CONFIG_SRC=	./usr.etc/config
MAKEINSTALL=	/usr/release/bin/makeinstall
WINSTALL=	/usr/release/bin/winstall
UNMOUNT	=	/usr/release/bin/unmount
INCDIR	=	/usr/tmp
CRYPT	=	sundist/exclude.lists/crypt.kit

.DEFAULT:
	sccs get $@

#
# Programs that live in subdirectories, and have makefiles of their own.
# Order here is critical for make install.
#
#	suninstall *must* come last
#	the libraries *must* come first
#
LAST=	usr.etc/suninstall
SUBDIR=	 etc bin usr.etc usr.bin ucb 5bin \
	xpginclude xpglib xpgbin posixlib adm files man pub sccs \
	diagnostics sys demo games old ${LAST}

#LIBDIR=	include lib usr.lib 5include 5lib
LIBDIR=	lib usr.lib ucblib
INCLDIR= include ucbinclude
CRYPTDIR= lib 5lib include include/rpcsvc bin etc ucb share \
	share/man share/man/man1 share/man/man3

#
# Directories that must exist before make install
#
DIRS=	etc tmp var var/adm var/crash var/log var/preserve var/spool var/tmp \
	dev mnt usr sbin home
USRDIRS= bin ucb etc include 5bin 5lib 5include lib share hosts \
	 boot local old kvm xpg2include xpg2lib xpg2bin

#
# The default target populates the /usr/src tree, installs the new header
# files, builds, tests, and installs the new language tools, and finally
# compiles all of /usr/src.
#
all:	INCBOOT LANGBOOT lang LIBRARIES ${SUBDIR}
	@${ECHO} "Build done on" `${DATE}` > BUILT
	@cat BUILT

BUILT:
	${MAKE} DESTDIR=${DESTDIR} CHOWN=${CHOWN} all

#########################
#
# HOW TO DO IT:
#
# At this writing, the recommended sequence of commands to produce
# an entire release starting in a bare directory is:
#
#	mkdir SCCS_DIRECTORIES
#	mount `pwd`/SCCS_DIRECTORIES
#	ln -s SCCS_DIRECTORIES/SCCS
#	make POPULATED
#	su -c "make makeinstall"
#	make all
#	su -c "make proto"

#########################
#
# STEP 0: RESOURCES
#
# Before going any further, check to ensure that we have the resources
# and environment we need to build the release.
#

.INIT: environment

environment:
	@k=`df ${INCDIR} |sed 1d|awk ' { print $$4 }'`;\
	if [ $$k -lt 4000 ] ; then \
		echo ${INCDIR} "does not have enough space in it!"; \
		echo "Make at least 4M available and try again."; \
		false;\
	elif [ `csh -c limit|grep stacksize|awk '{print $$2}'` -lt 24000 ]; \
	then \
		echo "Stacksize limit too low!"; \
		echo "Increase stack limit to at least 24M and try again."; \
		false; \
	fi
	
#########################
#
# STEP 1: POPULATE
#
# Assume a parallel hierarchy of SCCS files rooted at ./SCCS_DIRECTORIES.
# The POPULATE pass at least creates all the subdirectories and sets up
# the SCCS links. If a file named RELEASE exists in the current directory,
# and contains a string naming an SID-list file found in ./SIDlist,
# then all the source files needed to build that release will be checked out.
#

POPULATED:	populate
	${MAKE} clobber
	${SH} populate
	${MAKE} links
	${ECHO} "Source Tree populated on" `${DATE}` > $@

clobber:	unpopulate
	if [ -f RELEASE ]; then ${SH} unpopulate; fi
	${RM} -f POPULATED

links:		rmlinks 
	${CPP} makelinks | ${SH} -x

rmlinks:	makelinks
	${CPP} -DCLEAN makelinks | ${SH} -x

#########################
#
# STEP 1.5 UTIL_BOOT
#
# This step makes and installs from source all those odd ball utilities
# which, in the past, have had new features added which a current build
# would depend on. Because the new version was not installed before the
# build started, the build would come to a halt at the point where the
# new feature was needed. It is expected that this list may grow.
#
# Note: there may at some time develop a "chicken/egg" loop if any of
# these utilities themselves are changed such that they need a new or
# changed header file installed before it could correctly build (the
# new header files are installed after this step in STEP number 2).
# If this rare case arises, you will have to manually install the
# header file(s) before this step.
# Another case to consider is that of config. This program depends
# on yacc, lex, and libln. As these programs are pretty stable,
# and, as any changes to them will probably not affect config,
# it probably will not be a problem. Still, they must be considered
# in the worst case.

UTIL_BOOT:	POPULATED
	@$(MAKE) unmount
	cd ${INSTALL_SRC}; make installcmd; \
		${WINSTALL} installcmd /usr/bin/install;
	cd ${MAKE_SRC}; ${MAKEINSTALL} DESTDIR=
	cd ${SCCS_SRC}; ${MAKEINSTALL} DESTDIR=
	cd ${CONFIG_SRC}; ${MAKEINSTALL} DESTDIR=
	@${ECHO} "Made UTIL_BOOT on" `${DATE}` > $@

#
# Since UTIL_BOOT is the first target after the populate pass, it's here
# that we do the rest of the environmental setup. (We couldn't do it 
# sooner, or the populate pass would have clobbered anything we did.)
#
# We check to see if we're building an SID-based
# release. If so, then we need to unmount the SCCS directories
# before we actually start to build anything.
#

unmount:	$(MAKEINSTALL)
	@set -x; if [ -f RELEASE -a -d SCCS_DIRECTORIES/SCCS ]; then \
		${UNMOUNT} `pwd`/SCCS_DIRECTORIES; \
	fi

#
# Unfortunately, some parts of the build must be done as root.
# makeinstall and unmount are setuid-root scripts which are built
# and installed while running as root.
#
makeinstall:	$(MAKEINSTALL)

$(MAKEINSTALL):	POPULATED
	@if [ ! -u $(MAKEINSTALL) ]; then \
		${MAKE} rootid || (echo "You must, as root, do a \
		'make makeinstall'. \
		This will install /usr/release/bin with \
		the appropriate programs which allow a lang boot \
		to run" | fmt; false); \
		cd ${RELEASE_SRC}; $(MAKE) install; \
	else \
		cd ${RELEASE_SRC}; $(MAKEINSTALL); \
	fi

rootid:
	@if [ "`whoami`x" != "root"x ] ; then \
		false; \
	fi

#########################
#
# STEP 2: INCBOOT
#
# The reason we do this is to make sure all the
# include files are populated correctly. Also, they really have
# to overlay our running system. Right now, this involves
# going into the include directory and doing an install as root
# of include.
#

incboot:	INCBOOT

INCBOOT:	UTIL_BOOT
	@${MAKE} unmount
	cd include; ${MAKEINSTALL} DESTDIR=
	@${ECHO} "Made INCBOOT on" `${DATE}` > $@

#########################
#
# STEP 3: LANGBOOT
#
# This requires that the program /usr/release/bin/winstall
# exists. This program is suid root, because lang boot wipes the current
# system's libraries and compilers.
#

langboot:	LANGBOOT

LANGBOOT:	INCBOOT 
	@${MAKE} unmount
	@set -x;CWD=`pwd`;case `mach` in \
	mc68010|mc68020) \
		cd lang/boot; ${MAKE} $(MFLAGS) CPU=m68k SRCROOT=$$CWD;;\
	sparc) \
		cd lang/boot; ${MAKE} $(MFLAGS) CPU=sparc SRCROOT=$$CWD;;\
	esac
	@${ECHO} "Langboot done on" `${DATE}` > $@

#########################
#
# STEP 4: BUILD
#
# Generic rules to actually build the subdirectories.
#

lang: POPULATED 
	@${MAKE} unmount
	@set -x;CWD=`pwd`;case `mach` in \
	mc68010|mc68020)  cd lang; ${MAKE} $(MFLAGS) CPU=m68k  SRCROOT=$$CWD;; \
	sparc)		  cd lang; ${MAKE} $(MFLAGS) CPU=sparc SRCROOT=$$CWD;; \
	esac

#
# This is a terrible, terrible hack, and I apologize for it. The right
# thing to do is to set the LD_LIBRARY_PATH up front to point to
# the libraries under development. As soon as that's tested, this
# will be fixed.
#
LIBRARIES: POPULATED 
	@$(MAKE) unmount
	@set -x; for i in ${LIBDIR}; do \
		(cd $$i; $(MAKE) $(MFLAGS)); \
		(cd $$i; $(MAKEINSTALL) $(MFLAGS) DESTDIR= CHOWN=$(CHOWN)); \
	done
	@${ECHO} "Libraries done on" `${DATE}` > LIBRARIES

$(SUBDIR): POPULATED 
	@${MAKE} unmount
	cd $@; $(MAKE) ${MFLAGS}

#########################
#
# STEP 5: INSTALL
#
# The "proto" target is provided as a convenience; it's important to
# carefully clean out the /proto partition before building the /proto
# file system.
#

install:	BUILT install_dirs
	@set -x; for i in ${INCLDIR} ${LIBDIR} ${SUBDIR}; do \
	    (cd $$i; \
	    $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} CHOWN=$(CHOWN) install); \
	done
	@set -x; case `mach` in \
	mc680?0)    cd lang; \
		    $(MAKE) $(MFLAGS) install DESTDIR=$(DESTDIR) CPU=m68k;;\
	sparc)	    cd lang; \
		    $(MAKE) $(MFLAGS) install DESTDIR=$(DESTDIR) CPU=sparc;;\
	esac

install_dirs:
	@set -x; for i in ${DIRS}; do \
		install -d -o bin -m 755 ${DESTDIR}/$$i; \
	done
	@set -x; for i in ${USRDIRS}; do \
		install -d -o bin -m 755 ${DESTDIR}/usr/$$i; \
	done
	chmod 1777 ${DESTDIR}/tmp ${DESTDIR}/var/tmp
	${RM} -f ${DESTDIR}/bin && ln -s usr/bin ${DESTDIR}/bin
	${RM} -f ${DESTDIR}/lib && ln -s usr/lib ${DESTDIR}/lib
	${RM} -f ${DESTDIR}/sys && ln -s usr/kvm/sys ${DESTDIR}/sys
	${RM} -f ${DESTDIR}/usr/ucbinclude && ln -s include ${DESTDIR}/usr/ucbinclude
	${RM} -f ${DESTDIR}/usr/ucblib && ln -s lib ${DESTDIR}/usr/ucblib
	${RM} -f ${DESTDIR}/usr/src && ln -s share/src ${DESTDIR}/usr/src

userinstall:	${MAKEINSTALL}
	${MAKEINSTALL} DESTDIR=${DESTDIR} CHOWN=${CHOWN}

proto:		PROTO INTERNATIONAL NWZ CDROM SUNUPGRADE

PROTO:		BUILT
	@$(MAKE) rootid || (echo "You must make proto as root."; false)
	@${MAKE} unmount
	@RAW=`egrep /proto /etc/fstab | awk -F/ '{print "/dev/r"$$3}'`; \
		if [ ! -n "$$RAW" ]; then \
			echo "no device in fstab for /proto?"; \
			exit 1; \
		fi; \
		echo using $$RAW; \
		umount /proto; \
		newfs $$RAW; \
		mount /proto; \
		chmod g+s /proto; \
		chown root /proto; \
		chgrp staff /proto; \
		chmod g+s /proto/lost+found
	$(MAKE) install DESTDIR=/proto
	@echo "Prototype file system built on" `${DATE}` > $@
	@echo running ranlib -t on library archives under ${DESTDIR}/usr...
	@-for i in "`find ${DESTDIR}/usr -name '*lib*' -name '*\.*a*' \
		-type f -print`"; do \
		(file $$i | grep archive > /dev/null && ranlib -t $$i); \
	done
	@echo done

#########################
#
# STEP 6: TAPES
#
# This actually gets done in sundist, and is provided here as a convenience.
#
#
#tapes:		PROTO
#	@$(MAKE) rootid || (echo "You must make tapes as root."; false)
#	@${MAKE} unmount
#	@cd sundist; $(MAKE)
#	@cd sundist; $(MAKE) quarter_hd_tapes
#	@cd sundist; $(MAKE) half_tapes
#
#########################
#
# STEP 6: INTERNATIONALIZE
#

INTERNATIONAL:	international

international:  PROTO CRYPTKIT Makefile.inter 
	$(MAKE) $(MFLAGS) DESTDIR=$(DESTDIR) -f Makefile.inter international
	@echo "Internationalizationa done on " `${DATE}` > INTERNATIONAL

CRYPTKIT:	PROTO Cryptlist
	@if [ -f $@ ]; then \
		echo "$@ already exists; I mustn't clobber it."; \
		echo "Have you already run 'make international?'"; \
		false; \
	fi
	k=`cat Cryptlist`; (cd $(DESTDIR); tar cBfv - $$k) > $@

Cryptlist:	PROTO $(CRYPT)
	for i in `cat $(CRYPT)`; do \
		( cd $(DESTDIR); $(LS) $$i ); \
	done > $@

#########################
#
# STEP 7: Nuclear Waste Zone (NWZ)
#
# Move the patches up to proto
#

NWZ:	nwz

nwz:
	install -d /proto/UNDISTRIBUTED/patches
	cd /UNDISTRIBUTED; \
	find . -print | cpio -pdlm /proto/UNDISTRIBUTED/patches
	@echo "NWZ done on " `${DATE}` > NWZ

#########################
#
# STEP 8: CDROM
# CDROM image for this arch only
#

CDROM:	cdrom

cdrom:
	cd sundist; ${MAKE}
	cd sundist; ${MAKE} cdrom
	@echo "CDROM done on " `${DATE}` > CDROM

#########################
#
# STEP 9: sunupgrade
#
 
SUNUPGRADE:	sunupgrade

sunupgrade:
	cd sundist; $(MAKE) sunupgrade_target
	@echo "SUNUPGRADE done on " `${DATE}` > SUNUPGRADE

#########################
#
# STEP 10: PREINSTALL
#
# This is only used for machines which will ship a preinstalled distribution.
#

preinstall:	
	@${MAKE} unmount
	@cd sundist/pre; $(MAKE)
	@echo "See sundist/pre/build-from-suni for build instructions."

clean: POPULATED makelinks
	rm -f a.out core *.s *.o
	-for i in ${SUBDIR}; do (cd $$i; $(MAKE) ${MFLAGS} clean); done
	@set -x;case `mach` in \
		mc68010|mc68020)cd lang; $(MAKE) $(MFLAGS) clean CPU=m68k;;\
		sparc)		cd lang; $(MAKE) $(MFLAGS) clean CPU=sparc;;\
	esac

FRC:
