#
# @(#)Makefile 1.1 92/07/30 SMI
#

#  This makefile is still not perfect, but a "make clean" followed
#    by a "make" will rebuild everything.  "touch files" are used to
#    avoid reconfiguring on every "make".
#  Note that the targets are all actual files, so e.g., it's
#    $(ARCH)/LINT, not just LINT.

.SUFFIXES:

DESTDIR=
CHOWN=/etc/chown
CONFIG=/etc/config
ARCH:sh = arch -k

KERNELS=	GENERIC				\
		GENERIC_SMALL			\
		MINIROOT			\
		MUNIX

MAKEFILES=	kadb/Makefile			\
		boot/Makefile			\
		$(ARCH)/conf/Makefile

# those files that should cause a reconfigure (not the same as a rebuild)
#
CONFIGFILES=	$(ARCH)/conf/files		\
		$(ARCH)/conf/devices		\
		$(ARCH)/conf/Makefile.src	\
		$(ARCH)/conf/Makefile.obj	\
		conf.common/files.cmn		\
		conf.common/master

OTHERFILES=	$(ARCH)/genassym.c		\
		conf.common/Makefile.install	\
		conf.common/EXCEPTIONS		\
		conf.common/README		\
		conf.common/RELEASE		\
		conf.common/defines		\
		conf.common/mkconfig.sh		\
		conf.common/newvers.sh		\
		conf.common/param.c		\
		sun/swapgeneric.c		\
		sun/symbols.raw

MOSTFILES=	$(LIBDIR) $(SPECDIR) $(MAKEFILES) $(CONFIGFILES) $(OTHERFILES)

# touch files which need to be removed on a clean
#
TOUCHFILES=config_* configfiles headerfiles

# Various directories
#
KERNELDIRS=	$(KERNELS:%=$(ARCH)/%)
LINTDIR=	$(ARCH)/LINT
OTHER_KERNELS=	$(ARCH)/GENERIC_SMALL $(ARCH)/MUNIX $(ARCH)/MINIROOT

# Subdirectories that have makefiles of their own
# and need to have 'make depend' run before descending into them.
#
DEPENDS=kadb 

# Subdirectories (with makefiles) that have config dependencies.
#
CONFIGS=$(KERNELDIRS)

# Subdirectories (with makefiles) that have no dependencies or preconditions.
#
SIMPLES=boot adb

# Subdirectories with makefiles that implement clean and install
# The kernels are excluded:
#    clean requires config (which can take a very long time), and every
#    install is the same in every kernel makefile, so they would conflict
#
SUBDIR=	$(SIMPLES) $(DEPENDS) 

# Libraries which must be up to date before building the kernel.
# XXX we only need source and header files, not libraries
#
LIBDIR= des rpc rpcsvc boot/lib/$(ARCH)

# Directories which need special treatment
#
SPECDIR= pixrect sunwindow

# Encapsulate architectural dependencies here
#
ALL.ARCH= all.sun3 all.sun3x all.sun4 all.sun4c all.sun4m


all:	all.$(ARCH)

$(ALL.ARCH):	$(MOSTFILES) $(CONFIGS) ${SUBDIR} ${LINTDIR}

$(DEPENDS): FRC
	cd $@ && $(MAKE) ${MFLAGS} depend
	cd $@ && $(MAKE) ${MFLAGS}

$(LIBDIR) $(SIMPLES): FRC
	cd $@ && $(MAKE) ${MFLAGS}

pixrect: FRC
	cd ../usr.lib/libpixrect && $(MAKE) $(MFLAGS) kernel

sunwindow: FRC
	cd $@/rect && $(MAKE) $(MFLAGS) src
	cd $@ && $(MAKE) $(MFLAGS) hdr SUBDIRS="attr misc pw rect win"

$(CONFIGS): config_$$(@F) FRC
	cd $@ && $(MAKE) ${MFLAGS}

$(LINTDIR): config_GENERIC FRC
	install -d $(LINTDIR)
	cd $(ARCH)/conf && $(MAKE) $(MFLAGS) LINT && $(CONFIG) -g LINT
	cd $(LINTDIR) && $(MAKE) ${MFLAGS} lint | 2>&1 tee lint.errs
#
# Handle config dependencies
#
$(KERNELS:%=config_%): configfiles $(ARCH)/conf/$$(@F:config_%=%) 
	cd $(ARCH)/conf && \
		$(MAKE) $(MFLAGS) $(@F:config_%=%) && \
		${CONFIG} -g $(CONFIGFLAGS) $(@F:config_%=%)
	touch $@

$(KERNELS:%=$(ARCH)/conf/%):
	cd $(@D) && $(MAKE) $(MFLAGS) $(@F)

#
# These kludges exist because:
#	* the way some symbolic links are handled
#	* some header files are not retrieved when the install_h target is run
#	* the files needed to run config are thrown in here
#	* not all files are SCCS'ed 
#
configfiles: headerfiles $(CONFIGFILES)
	touch $@

headerfiles:
	cd krpc &&	$(MAKE) $(MFLAGS)
	cd specfs &&	$(MAKE) $(MFLAGS)
	cd m68k &&	$(MAKE) $(MFLAGS)
	cd sparc &&	$(MAKE) $(MFLAGS)
	cd pcfs &&	$(MAKE) $(MFLAGS)
	cd debug &&	$(MAKE) $(MFLAGS)
	cd lwp &&	$(MAKE) $(MFLAGS) kernel
	touch $@
#
# End of get kludges
#

install:
	set -x;for i in ${SUBDIR} ${OTHER_KERNELS}; do \
		(cd $$i && $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install); done
	cd $(ARCH)/conf && $(MAKE) $(MFLAGS) DESTDIR=${DESTDIR} install

clean: $(MAKEFILES) FRC
	$(RM) a.out core *.s *.o $(TOUCHFILES)
	@set -x;for i in ${SUBDIR} $(LINTDIR) $(OTHER_KERNELS); \
	do ( if [ -d $$i ];then cd $$i && $(MAKE) ${MFLAGS} clean;fi;);done

#
# Install header files
#
# network directories...
#
NETDIRS=net netinet nettli
#
# include all filesystem directories
#
FSDIRS= lofs tfs specfs nfs rfs ufs tmpfs hsfs pcfs
SYSDIRS=$(NETDIRS) $(FSDIRS) debug m68k sparc sys vm os
SUNSYSDIRS=sbusdev scsi sun sun3 sun3x sun4 sun4c sun4m \
		sundev sunif sunwindowdev 
MACHSYSDIRSsun3=$(SUNSYSDIRS)
MACHSYSDIRSsun3x=$(SUNSYSDIRS)
MACHSYSDIRSsun4=$(SUNSYSDIRS)
MACHSYSDIRSsun4c=$(SUNSYSDIRS)
MACHSYSDIRSsun4m=$(SUNSYSDIRS)
MACHSYSDIRSvax=vax vaxif vaxmba vaxuba
STANDDIRS=boot stand/mon

install_h: headerfiles FRC
	set -x; for i in $(SYSDIRS) $(MACHSYSDIRS$(ARCH)); do \
		install -d -o bin -m 755 $(DESTDIR)/usr/include/$$i; \
		( cd $$i && \
		$(MAKE) $(MFLAGS) $@ CHOWN=$(CHOWN) DESTDIR=$(DESTDIR) ); \
        done
	@set -x; for i in $(STANDDIRS); do \
		( cd $$i && \
		$(MAKE) $(MFLAGS) $@ CHOWN=$(CHOWN) DESTDIR=$(DESTDIR) ); \
	done

#
# Make a tags file and set up links to it from all kernel
# source directories.
#
tags: FRC
	cd $(ARCH)/GENERIC && ${MAKE} $(MFLAGS) tags
	m=$(ARCH); \
	for i in `ls -d */. | grep -v "^$$m/."`; do \
		$(RM) $$i/tags; ln -s ../$$m/tags $$i/tags; \
	done

#
# FoRCing target
#
FRC:
