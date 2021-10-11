#
#  @(#)Makefile 1.1 92/07/30 SMI
#

# NOT in /proto!!!
# upgrade uses tarfiles, so we are in a chiken_and_eggs loop.
# We can't put it in /proto.
DESTDIR = /proto_sunupgrade

# subdirectories that have its own makefile.
SUBDIR = file_db file_db.b file_db.412 shell tools

SHELLSC = compar_A_B
TEXTFILES = _readme volatile_list

# An empty directory that will be filled by the outputs of compar_A_B later.
INCLD = incld

RELEASE = /usr/src/sys/conf.common/RELEASE
RELEASE411 = sunos_4_1_1
RELEASE412 = sunos_4_1_2
RELEASE_NEW = "sunos_4_1_`cut -c5- ${RELEASE} | tr '.' '_' | tr '[A-Z]' '[a-z]'`"
# MEDIA411/MEDIA412 is pretty much like a root point of a 411/412 sunos 
# cdrom file system, except that only export directory is left in it. 
# Also in order to speed up build time, we un-tar'ed each package in 
# advance and put untarfiles under an extra directory named after that 
# package in the dircetory the package used to be.
# Currently 411 un-tar'ed packages are stored in 
# eichler:/411_sunupgrade/untarfiles and 412 un-tar'ed packages are
# stored in eichler:/412_sunupgrade/untarfiles.
# Sun4 & sun4c & sun4m build machines should have them mounted to 
# /411_cdrom_untar and /412_cdrom_untar.
MEDIA411 = /411_cdrom_untar
MEDIA412 = /412_cdrom_untar

MEDIA_NEW = /tarfiles
	
# Make sure we have about 30MB available in /sunupgrade_tmp. 
# Sunupgrade will use /sununupgrade_tmp/tmp 
# to contain intermediate output for comparison of 413 tarfiles.
TMPDIR = /sunupgrade_tmp/tmp

KARCH = `arch -k`
# This is a list of packages which compar_A_B compares between 411 & 413
# and 412 & 413.
# The list is copied from /tarfiles/sun4* on build machine.
# To speed up build time, we broke the package list to three so that
# sun4, sun4c and sun4m can do comparison parallelly.
PACK_LIST_sun4="User_Diag Sys Kvm root Debugging Demo Games Graphics\
		Install Manual Networking RFS Security Shlib_Custom\
		SunView_Demo SunView_Programmers SunView_Users"

PACK_LIST_sun4c="usr Sys Kvm Text TLI"

PACK_LIST_sun4m="Sys Kvm Versatec System_V uucp"

# A change in the root package of 411_rev_B only.
ROOT_CHANGE_411_REVB_ONLY = root_change_rev_b_only
# A change in the kvm package of 411_rev_B only.
SHOWREV_DOT_DAT = showrev_dot_dat

# special files
SPECIAL_FILES = ${ROOT_CHANGE_411_REVB_ONLY} ${SHOWREV_DOT_DAT}

# This is a file will be created by compar_A_B. It tells the size difference
# between each package of 411 & 413 or 412 & 413.
SIZE = _size_diff

# This is just a log for script compar_A_B.
CMPLOG = /sunupgrade_tmp/cmplog

all:	make_tmp_dir ${SHELLSC} ${TEXTFILES} ${SUBDIR}

make_tmp_dir:  $(TMPDIR)

$(TMPDIR):
	mkdir $(TMPDIR); chmod a+w $(TMPDIR)

${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

# Make sure 4.1.3 tarfiles are available by now.
${SHELLSC}:	${SPECIAL_FILES}
	rm -f $@
	cp -p $@.sh $@
	chmod +x $@
	install -d -m 755 ${DESTDIR}
	install -d -m 755 ${DESTDIR}/${INCLD}
	if sun4; then \
		$@ ${KARCH} ${RELEASE411} ${RELEASE_NEW} ${MEDIA411}\
			${MEDIA_NEW} ${TMPDIR}\
			${DESTDIR}/${INCLD} ${CMPLOG} ${PACK_LIST_sun4}; \
		install -m 755 ${ROOT_CHANGE_411_REVB_ONLY}\
			${DESTDIR}/${INCLD}/proto_root_${RELEASE_NEW}.delete;\
		install -m 755 ${SHOWREV_DOT_DAT}\
			${DESTDIR}/${INCLD}/${KARCH}_${RELEASE_NEW}.kvm.delete;\
	fi
	if sun4c; then \
		$@ ${KARCH} ${RELEASE411} ${RELEASE_NEW} ${MEDIA411}\
			${MEDIA_NEW} ${TMPDIR}\
			${DESTDIR}/${INCLD} ${CMPLOG} ${PACK_LIST_sun4c}; \
		install -m 755 ${ROOT_CHANGE_411_REVB_ONLY}\
			${DESTDIR}/${INCLD}/proto_root_${RELEASE_NEW}.delete;\
		install -m 755 ${SHOWREV_DOT_DAT}\
			${DESTDIR}/${INCLD}/${KARCH}_${RELEASE_NEW}.kvm.delete;\
	fi
	if sun4m; then \
		$@ ${KARCH} ${RELEASE412} ${RELEASE_NEW} ${MEDIA412}\
			${MEDIA_NEW} ${TMPDIR}\
			${DESTDIR}/${INCLD} ${CMPLOG} ${PACK_LIST_sun4m}; \
	fi
install: FRC
	for i in $(TEXTFILES); do \
		install -m 755 $$i ${DESTDIR}/$$i; \
	done
	install -m 755 ${RELEASE} ${DESTDIR}/release
	@set -x; for i in ${SUBDIR}; do \
	(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); done

# Since sun4, sun4c and sun4m are sharing /usr/src/sundist/sunupgrade/incld
# and /usr/src/sundist/proto_sunupgrade, the clean-up needs to be handled
# in compar_A_B package by package.
# We are not taking care of that here.
clean:
	rm -fr /sunupgrade_tmp/*
	rm -f $(SHELLSC) 
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done

FRC:
