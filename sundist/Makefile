#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR=
CFLAGS=	-O
LDFLAGS=
CHOWN=	/etc/chown
CLEANUPCPP=sed -e '/^\#.*/d' -e '/^$$/d'
PROTO=	/proto
TARFILES= /tarfiles
ARCH:sh = arch -k
# incomplete dependancy list!
BIN=	bin/mktp bin/buildmini bin/getmini bin/getmunixfs \
	bin/getmini.internal
MKTP_INPUTS=	sun3_quarter	sun3_half	\
		sun3x_quarter	sun3x_half	\
		sun4_quarter	sun4_half	\
		sun4m_quarter	sun4m_half	\
		sun4c_quarter	sun4c_half	\
		sun4c_diskette	\
		sun3_cdrom sun3x_cdrom sun4_cdrom sun4c_cdrom sun4m_cdrom
		
MAKEINSTALL=/usr/release/bin/makeinstall 

PART_FILES=	part_num_quarter_sun3 part_num_quarter_sun3x \
		part_num_quarter_sun4 part_num_quarter_sun4c \
		part_num_half_sun3    part_num_half_sun3x \
		part_num_half_sun4    part_num_half_sun4c \
		part_num_half_sun4m   part_num_quarter_sun4m \
		part_num_diskette_sun4c \
		part_num_cdrom

CD_FILES=       Makefile.common_cd cdrom.mk cdimage.mk \
		cdpatch.README README_cdimage

#
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	mktp util exclude.lists mkcd

# sparc binaries need a bigger file system 
#
MUNIXFS_SIZE-sparc = 4096
MUNIXFS_SIZE-mc68010 = 3072
MUNIXFS_SIZE-mc68020 = 3072
MUNIXFS_SIZE-mc68030 = 3072
MUNIXFS_SIZE = $(MUNIXFS_SIZE$(TARGET_MACH))

MUNIXFS_NBPI_DEFAULT = 2048
MUNIXFS_NBPI-sparc = 1920
MUNIXFS_NBPI-mc68010 = $(MUNIXFS_NBPI_DEFAULT)
MUNIXFS_NBPI-mc68020 = $(MUNIXFS_NBPI_DEFAULT)
MUNIXFS_NBPI-mc68030 = $(MUNIXFS_NBPI_DEFAULT)
MUNIXFS_NBPI = $(MUNIXFS_NBPI$(TARGET_MACH))

# Text files that live in the current directory
#
TEXT= README Copyright

COMMON= bin/mktp miniroot.$$(ARCH) DISKSIZES.$$(ARCH)
SUNUPGRADE= sunupgrade
DESTDIR_SUNUPGRADE= /proto_sunupgrade

all:	${SUBDIR} ${TEXT} ${PART_FILES} ${BIN} ${CD_FILES} proto_loaded_ow proto_loaded_xgl

#
# XXX temp hack for OpenWindows in Pie
#
ARCH_TYPE=    `arch`
OW_DIR=       ${PROTO}/usr/openwin
SRC_DATA=     /openwin_files/${ARCH_TYPE}
COPTIONS=     cf
XOPTIONS=     xpf
DEST_XLIST=   ./exclude.lists
USR_XLIST=    usr.ow_usr
PROG_XLIST=   usr.ow_prog
DEMO_XLIST=   usr.ow_demo
FONT_XLIST=   usr.ow_font
SUBSETS=   -I /openwin_files/_install/${REQUIRED} \
	-I /openwin_files/_install/${HANDBOOKS} \
	-I /openwin_files/_install/${MAN} \
	-I /openwin_files/_install/${MAN3} \
	-I /openwin_files/_install/${FONTS} \
	-I /openwin_files/_install/${MIT}  \
	-I /openwin_files/_install/${DEMO}  \
	-I /openwin_files/_install/${IMAGES} \
	-I /openwin_files/_install/${INCLUDE} \
	-I /openwin_files/_install/${LINTLIBS} \
	-I /openwin_files/_install/${STATICLIBS}\
	-I /openwin_files/_install/${SAMPLESRC}

REQUIRED=	package.required.sun4
HANDBOOKS=	package.handbooks.sun4
MAN=		package.man.sun4
MAN3=		package.man3.sun4
FONTS=		package.fonts.sun4
MIT=		package.mitfrills.sun4
DEMO=		package.demos.sun4
IMAGES=		package.images.sun4
INCLUDE=	package.include.sun4
LINTLIBS=	package.lintlibs.sun4
STATICLIBS=	package.staticlibs.sun4
SAMPLESRC=	package.samplesrc.sun4

proto_loaded_ow:
	( cd /openwin_files/_install; \
		cat ${REQUIRED} ${HANDBOOKS} ${MAN} ${MIT} ) | \
		sed 's/^\./\.\/openwin/' > ${DEST_XLIST}/${USR_XLIST}
	( cd /openwin_files/_install; cat ${DEMO} ${IMAGES} ) | \
		sed 's/^\./\.\/openwin/' > ${DEST_XLIST}/${DEMO_XLIST}
	( cd /openwin_files/_install; cat ${FONTS} ) | \
		sed 's/^\./\.\/openwin/' > ${DEST_XLIST}/${FONT_XLIST}
	( cd /openwin_files/_install; cat ${MAN3} ${INCLUDE} ${LINTLIBS} ${STATICLIBS} ${SAMPLESRC} )| \
		sed 's/^\./\.\/openwin/' > ${DEST_XLIST}/${PROG_XLIST}

	if [ ! -d ${OW_DIR} ] ; \
	then \
		mkdir ${OW_DIR} ; \
		chmod 755 ${OW_DIR} ; \
	fi
	( cd ${OW_DIR}; rm -fr *); \
	((cd ${SRC_DATA}; tar ${COPTIONS} - ${SUBSETS})| (cd ${OW_DIR}; tar ${XOPTIONS} -))

# XXX------------------------------------------------------------

#
# YYY temp hack for XGL in Pie
#
XGL_DIR=     ${PROTO}/usr
XGL_SRC=     /xgl_file/${ARCH_TYPE}
XGL_LIST=    xgl.list
XGL_FILES=   include lib

proto_loaded_xgl: ${XGL_LIST} ${XGL3_LIST} FRC
	if [ ! -d ${XGL_DIR} ] ; \
	then \
		mkdir ${XGL_DIR} ; \
		chmod 755 ${XGL_DIR} ; \
	fi
	( lst=`pwd`/${XGL_LIST} ; \
	cd ${XGL_DIR}; \
	(cd ${XGL_SRC}; tar cf - ${XGL_FILES}) | tar xpf - -I $$lst ) ; \
	
# YYY--------------------------------------------------------------

${SUBDIR}: FRC
	cd $@; $(MAKE) ${MFLAGS}

#
# Take note that if MINISIZE is to be changed, the same change must
# also be made in file /usr/src/sys/vm/vm_swap.c
#
miniroot.$(ARCH): $(PROTO) $(BIN) Makefile
	MINISIZE=14000 &&\
	MINIARCH=$(ARCH) &&\
	MININAME=`pwd`/miniroot.$$MINIARCH &&\
	MINIGET=`pwd`/bin/getmini &&\
	export MINISIZE MINIARCH MININAME MINIGET &&\
	bin/buildmini

munixfs.$(ARCH).tape: $(PROTO) $(BIN) Makefile
	MINIMEDIATYPE=tape &&\
	MINISIZE=$(MUNIXFS_SIZE) &&\
	MINIARCH=$(ARCH) &&\
	MININAME=`pwd`/munixfs.$$MINIARCH.tape &&\
	MINIGET=`pwd`/bin/getmunixfs &&\
	NBPI=$(MUNIXFS_NBPI) && \
	export MINIMEDIATYPE MINISIZE NBPI MINIARCH MININAME MINIGET &&\
	bin/buildmini

# the generic minisize is impossibly large - it can't be bigger than
# 2880 to fit on a 3.5" diskette partition "c".
munixfs.$(ARCH).diskette: $(PROTO) $(BIN) Makefile
	MINIMEDIATYPE=diskette &&\
	MINISIZE=2880 &&\
	MINIARCH=$(ARCH) &&\
	MININAME=`pwd`/munixfs.$$MINIARCH.diskette &&\
	MINIGET=`pwd`/bin/getmunixfs &&\
	NBPI=4096 &&\
	export MINIMEDIATYPE MINISIZE NBPI MINIARCH MININAME MINIGET &&\
	bin/buildmini

# the cdrom munixfs made so that it knows about the cdrom
# needs to know how big the miniroot is
munixfs.$(ARCH).cdrom: $(PROTO) $(BIN) Makefile miniroot.$(ARCH)
	MINIMEDIATYPE=cdrom &&\
	MINISIZE=$(MUNIXFS_SIZE) &&\
	MINIARCH=$(ARCH) &&\
	MININAME=`pwd`/munixfs.$$MINIARCH.cdrom &&\
	MINIGET=`pwd`/bin/getmunixfs &&\
	NBPI=$(MUNIXFS_NBPI) && \
	export MINIMEDIATYPE MINISIZE NBPI MINIARCH MININAME MINIGET &&\
	bin/buildmini

#
# targets for MUNIX - only sun4c used now, later we will add rules
# for other architectures so that they will end up ready to go onto
# the media from $(TARFILES)/$(ARCH)_$(RELEASE)/tools/munix...
munix.tape.sun3 munix.tape.sun3x munix.tape.sun4:

munix.tape.sun4c munix.tape.sun4m: munixfs.${ARCH}.tape
	cp ${PROTO}/usr/kvm/stand/munix munix.tape.${ARCH}
	`pwd`/bin/stamprd munix.tape.${ARCH} munixfs.${ARCH}.tape
	rootfs="4.2" &&\
	rootdev="rd0a" &&\
	swapfs="spec" &&\
	swapdev="ns0b" &&\
	( ( echo "rootfs?W '$${rootfs}'" ; \
	  echo "rootfs+10?W '$${rootdev}'" ; \
	  echo "swapfile?W '$${swapfs}'" ; \
	  echo "swapfile+10?W '$${swapdev}'" ; \
	  echo "ramdiskispreloaded?W 1" ; \
	  echo '$q'; ) | adb -w munix.tape.${ARCH} ) >/dev/null 2>&1

TOSIZE.sun4m= sun4m_quarter sun4m_half sun4m_cdrom
TOSIZE.sun4c= sun4c_quarter sun4c_half sun4c_cdrom sun4c_diskette
TOSIZE.sun4= sun4_quarter sun4_half sun4_cdrom
TOSIZE.sun3= sun3_quarter sun3_half sun3_cdrom
TOSIZE.sun3x= sun3x_quarter sun3x_half sun3x_cdrom
TOSIZE= $(TOSIZE.$(ARCH))

# depends on existance of tar files, used by diskette and (now) tapes
COMPRESSED: COMPRESSED.$(ARCH) TARFILES.$(ARCH)

COMPRESSED.$(ARCH):
	-rm -rf $(TARFILES)/$(ARCH).Z
	mkdir $(TARFILES)/$(ARCH).Z
	# XXX should test for enuff space
	list=`cd $(TARFILES)/$(ARCH); ls $(TARFILES)/$(ARCH)` && \
	for i in $$list; do \
		compress -c $(TARFILES)/$(ARCH)/$$i > $(TARFILES)/$(ARCH).Z/$$i  ;\
	done;
	date > COMPRESSED.$(ARCH)

# create size info files and also the tar images
TARFILES.$(ARCH): $(PROTO) $(TOSIZE) munixfs.$(ARCH).tape tarfiles.awk
	-rm -rf $(TARFILES)/$(ARCH)/*
	-rm -rf COMPRESSED.$(ARCH) TARFILES.$(ARCH) DISKSIZES.$(ARCH)
	-mkdir $(TARFILES)/$(ARCH)
	# XXX should check for enuff space in $(TARFILES)/$(ARCH)
	#create shell commands to do create tarfiles
	-rm tarcmds.tmp
	cat $(TOSIZE) | awk -f tarfiles.awk dir=$(TARFILES)/$(ARCH) - | sort -u - > tarcmds.tmp
	PATH=`pwd`/bin:/usr/bin:/usr/etc && export PATH &&\
	ARCH=$(ARCH) && export ARCH && \
	sh tarcmds.tmp
	date > TARFILES.$(ARCH)

# Make sure all tarfiles have been installed into /tarfiles by now, as
# sunupgrade needs it to run.
sunupgrade_target:
	cd ${SUNUPGRADE}; ${MAKE} ${MFLAGS} clean
	cd ${SUNUPGRADE}; ${MAKE} ${MFLAGS}
	cd ${SUNUPGRADE}; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR_SUNUPGRADE} install

DISKSIZES.$(ARCH): TARFILES.$(ARCH) disksize.awk
	-rm -rf disksizes.$(ARCH)
	-rm -rf DISKSIZES.$(ARCH)
	mkdir disksizes.$(ARCH)
	#create shell commands to do disksizeing
	-rm sizecmds.tmp
	cat $(TOSIZE) | awk -f disksize.awk dir=disksizes.$(ARCH) - | sort -u - > sizecmds.tmp
	PATH=`pwd`/bin:/usr/bin:/usr/etc && export PATH &&\
	ARCH=$(ARCH) && export ARCH && \
	TARFILES=$(TARFILES) && export TARFILES && \
	sh -x sizecmds.tmp
	date > DISKSIZES.$(ARCH)


quarter_tapes: $(ARCH)_quarter munixfs.$(ARCH).tape $(COMMON) COMPRESSED munix.tape.$(ARCH)
	@$(MAKE) rootid
	ARCH=$(ARCH) && TARFILES=$(TARFILES) && export ARCH TARFILES && \
	PATH=`pwd`/bin:/usr/bin:/usr/etc && export PATH &&\
	bin/mktp $(ARCH)_quarter

half_tapes: $(ARCH)_half munixfs.$(ARCH).tape $(COMMON) COMPRESSED munix.tape.$(ARCH)
	@$(MAKE) rootid
	ARCH=$(ARCH) && TARFILES=$(TARFILES) && export ARCH TARFILES && \
	PATH=`pwd`/bin:/usr/bin:/usr/etc && export PATH &&\
	bin/mktp $(ARCH)_half

diskettes: $(ARCH)_diskette munixfs.$(ARCH).diskette $(COMMON) COMPRESSED
	@$(MAKE) rootid
	ARCH=$(ARCH) && TARFILES=$(TARFILES) && export ARCH TARFILES && \
	PATH=`pwd`/bin:/usr/bin:/usr/etc:/usr/ucb && export PATH &&\
	bin/mktp -t $(ARCH)_diskette
	# now make the diskettes themselves, too hard to do in mktp!
	make -f diskette.mk ARCH=$(ARCH) PROTO=$(PROTO) TARFILES=$(TARFILES) dist

cdrom: $(ARCH)_cdrom munixfs.$(ARCH).cdrom $(COMMON) 
	@$(MAKE) rootid
	ARCH=$(ARCH) && TARFILES=$(TARFILES) && export ARCH TARFILES && \
	PATH=`pwd`/bin:/usr/bin:/usr/etc:/usr/ucb && export PATH &&\
	bin/mktp -t $(ARCH)_cdrom
	# now make the diskettes themselves, too hard to do in mktp!
	make -f cdrom.mk ARCH=$(ARCH) PROTO=$(PROTO) TARFILES=$(TARFILES) deliverables

sun3_quarter sun3x_quarter: Makefile mktp_input part_num_quarter_$(ARCH)
	/usr/lib/cpp \
	-D$(ARCH) \
	-DTAPE \
	mktp_input |\
	/usr/lib/cpp \
	-DFILETYPE=tarZ \
	-DVERS="`cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE`" \
	-DDATE="`date`" \
	-DPART="`cat part_num_quarter_$(ARCH)`" \
	-DMEDIA="1/4\\\" tape QIC-24 \(boot format\)" \
	-DARCH=$(ARCH) \
	-DDEVRNAME=st8 \
	-DDEVSIZE=58mb \
	-DFSBLOCKING=100k \
	-DTARBLOCKING=200 \
	-DMINIROOT=miniroot.ARCH \
	-DMUNIXFS=munixfs.ARCH.tape \
	-DMUNIXFSBLOCKING=100k \
	-DPROTO=$(PROTO)  | $(CLEANUPCPP) > $@

sun4_quarter sun4c_quarter sun4m_quarter: Makefile mktp_input \
	part_num_quarter_$(ARCH)
	/usr/lib/cpp \
	-D$(ARCH) \
	-DTAPE \
	mktp_input |\
	/usr/lib/cpp \
	-DFILETYPE=tarZ \
	-DVERS="`cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE`" \
	-DDATE="`date`" \
	-DPART="`cat part_num_quarter_$(ARCH)`" \
	-DMEDIA="1/4\\\" tape QIC-24 \(boot format\)" \
	-DARCH=$(ARCH) \
	-DDEVRNAME=st8 \
	-DDEVSIZE=58mb \
	-DFSBLOCKING=100k \
	-DTARBLOCKING=200 \
	-DMINIROOT=miniroot.ARCH \
	-DMUNIXFS=munixfs.ARCH.tape \
	-DMUNIXFSBLOCKING=100k \
	-DPROTO=$(PROTO)  | $(CLEANUPCPP) > $@

sun3x_half sun3_half: Makefile mktp_input part_num_half_$(ARCH)
	/usr/lib/cpp \
	-D$(ARCH) \
	-DTAPE \
	mktp_input |\
	/usr/lib/cpp \
	-DFILETYPE=tarZ \
	-DVERS="`cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE`" \
	-DDATE="`date`" \
	-DPART="`cat part_num_half_$(ARCH)`" \
	-DMEDIA="1/2\\\" tape 1600 bpi \(boot format\)" \
	-DARCH=$(ARCH) \
	-DDEVRNAME=mt0 \
	-DDEVSIZE=37mb \
	-DFSBLOCKING=10k \
	-DTARBLOCKING=20 \
	-DMINIROOT=miniroot.ARCH \
	-DMUNIXFS=munixfs.ARCH.tape \
	-DMUNIXFSBLOCKING=8k \
	-DPROTO=$(PROTO)  | $(CLEANUPCPP) > $@

sun4_half sun4c_half sun4m_half: Makefile mktp_input part_num_half_$(ARCH)
	/usr/lib/cpp \
	-D$(ARCH) \
	-DTAPE \
	mktp_input |\
	/usr/lib/cpp \
	-DFILETYPE=tarZ \
	-DVERS="`cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE`" \
	-DDATE="`date`" \
	-DPART="`cat part_num_half_$(ARCH)`" \
	-DMEDIA="1/2\\\" tape 6250 bpi \(boot format\)" \
	-DARCH=$(ARCH) \
	-DDEVRNAME=mt8 \
	-DDEVSIZE=137mb \
	-DFSBLOCKING=10k \
	-DTARBLOCKING=20 \
	-DMINIROOT=miniroot.ARCH \
	-DMUNIXFS=munixfs.ARCH.tape \
	-DMUNIXFSBLOCKING=8k \
	-DPROTO=$(PROTO)  | $(CLEANUPCPP) > $@

# note that a lot of things are really set in diskette.mk
$(ARCH)_diskette: Makefile mktp_input part_num_diskette_$(ARCH)
	/usr/lib/cpp \
	-D$(ARCH) \
	-DDISKETTE \
	mktp_input |\
	/usr/lib/cpp \
	-DFILETYPE=tarZ \
	-DVERS="`cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE`" \
	-DDATE="`date`" \
	-DPART="`cat part_num_diskette_$(ARCH)`" \
	-DMEDIA="Diskette \(suninstall format\)" \
	-DARCH=$(ARCH) \
	-DDEVRNAME=rfd0a \
	-DDEVSIZE=1422kb \
	-DMINIROOT=miniroot.ARCH \
	-DMUNIXFS=munixfs.ARCH.diskette \
	-DMUNIXFSBLOCKING=8k \
	-DPROTO=$(PROTO)  | $(CLEANUPCPP) > $(ARCH)_diskette

# for cdrom, all archs
$(ARCH)_cdrom: Makefile mktp_input part_num_cdrom
	/usr/lib/cpp \
	-D$(ARCH) \
	-DCDROM \
	mktp_input |\
	/usr/lib/cpp \
	-DFILETYPE=tar \
	-DVERS="`cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE`" \
	-DDATE="`date`" \
	-DPART="`cat part_num_cdrom`" \
	-DMEDIA="CDROM \(suninstall format\)" \
	-DARCH=$(ARCH) \
	-DDEVRNAME=rsr0 \
	-DDEVSIZE=500mb \
	-DMINIROOT=miniroot.ARCH \
	-DMUNIXFS=munixfs.ARCH.cdrom \
	-DMUNIXFSBLOCKING=8k \
	-DPROTO=$(PROTO)  | $(CLEANUPCPP) > $(ARCH)_cdrom

$(BIN) install: $(MAKEINSTALL)
	install -d bin 
	for i in ${SUBDIR}; do \
		(cd $$i; $(MAKEINSTALL) DESTDIR=../bin); done

cd_populate: $(CD_FILES) part_num_cdrom

clean:
	$(RM) -rf errs bin miniroot.* munixfs.* $(MKTP_INPUTS) *.xdr \
		disksizes.$(ARCH) $(TARFILES)/$(ARCH) $(TARFILES)/$(ARCH).Z \
		TARFILES.$(ARCH) DISKSIZES.$(ARCH) COMPRESSED.$(ARCH) *.tmp
	for i in ${SUBDIR}; do (cd $$i; $(MAKE) ${MFLAGS} clean); done
	make -f diskette.mk ARCH=$(ARCH) clean
	make -f cdrom.mk ARCH=$(ARCH) clean

$(PROTO):

$(MAKEINSTALL):
	cd ..; $(MAKE) $(@F)

rootid:
	@if [ "`whoami`x" != "root"x ] ; then \
		echo "You must make the release media as \"root\"."; \
		false; \
	fi

FRC:
