#! /bin/sh
#
#	%Z%%M% %I% %E% SMI
#
# Shell script to build a MUNIX root file system
# in preparation for building a distribution tape.
# The file system created here is image copied onto
# tape, then image copied into the ram disk device as the "first"
# step in a cold boot of 4.1b-4.2 systems.
#
# combined for all medias, depends on MINIMEDIATYPE being set to
# "tape", "diskette", "cdrom" to do the right thing.
#
# make DAMN SURE when you change this file that it all still fits
# on the floppy, use "if [ $MINIMEDIATYPE = diskette ] ..." if needed.
#
DISTROOT=/proto
SUNDISTPATH=`dirname $0`
#
if [ `pwd` = '/' ]
then
	echo You almost destroyed the root
	exit
fi
date > .MUNIXFS
rm -rf bin; mkdir bin
rm -rf sbin; mkdir sbin
rm -rf etc; mkdir etc
rm -rf stand; mkdir stand
rm -rf a; mkdir a
rm -rf tmp; mkdir tmp; chmod 777 tmp
rm -rf usr 
mkdir usr usr/tmp usr/ucb usr/bin usr/lib usr/etc usr/kvm
mkdir usr/share
mkdir usr/share/lib
mkdir usr/share/lib/keytables
mkdir usr/share/lib/zoneinfo
ln -s usr/lib lib
#toss lost+found - save 16 blocks
rm -rf lost+found

# Copy dynamic loading and shared library environment.  
# Note that any shared library on which the components
# of the mini-root depends must be included here.
cp $DISTROOT/lib/ld.so lib
cp $DISTROOT/lib/libc.so.*.* lib
cp $DISTROOT/lib/libc.sa.*.* lib
ranlib lib/libc.sa.*.*
cp $DISTROOT/lib/libdl.so.*.* lib

# Copy individual components.
# 
# These WERE being re-dynamically linked.
# BUT: it doesn't make sense - you still need libc for all the rest
if [ $MINIMEDIATYPE = "tape" -o $MINIMEDIATYPE = "cdrom" ]
then
	cp $DISTROOT/bin/tar bin; strip bin/tar
	cp $DISTROOT/bin/mv bin; strip bin/mv
	cp $DISTROOT/usr/bin/ln bin; strip bin/ln
	cp $DISTROOT/etc/ifconfig etc; strip etc/ifconfig
fi
# Copy the rest
cp $DISTROOT/etc/mtab etc
cp /dev/null etc/rc.boot
cp $DISTROOT/usr/etc/dkinfo etc; strip etc/dkinfo
cp $DISTROOT/usr/etc/init sbin; strip sbin/init
cp $DISTROOT/usr/etc/mknod usr/etc; strip usr/etc/mknod
cp $DISTROOT/usr/etc/fsirand etc; strip etc/fsirand
cp $DISTROOT/usr/etc/format usr/etc; strip usr/etc/format
cp $DISTROOT/etc/format.dat etc
cp $DISTROOT/usr/etc/reboot usr/etc; strip usr/etc/reboot
if [ $MINIARCH = "sun4c" -o $MINIARCH = "sun4m" ]; then
	cp $DISTROOT/usr/kvm/unixname2bootname usr/kvm
fi
#
# link these instead of copying, save 9 blocks
cp $DISTROOT/usr/bin/true bin
cp $DISTROOT/usr/bin/false bin
ln bin/false etc/rc			# quiet init about rc not found
ln bin/false bin/sun2			# adios
ln bin/false bin/sun386			# FIXME
ln bin/false bin/i386			# FIXME
if [ $MINIARCH = "sun4c" ]; then
	ln bin/false bin/sun3
	ln bin/false bin/sun3x
	ln bin/false bin/m68k
	ln bin/false bin/sun4
	ln bin/false bin/sun4m
	ln bin/true bin/sun4c
	ln bin/true bin/sparc
elif [ $MINIARCH = "sun4m" ]; then
	ln bin/false bin/sun3
	ln bin/false bin/sun3x
	ln bin/false bin/m68k
	ln bin/false bin/sun4
	ln bin/false bin/sun4c
	ln bin/true bin/sun4m
	ln bin/true bin/sparc
elif [ $MINIARCH = "sun4" ]; then
	ln bin/false bin/sun3
	ln bin/false bin/sun3x
	ln bin/false bin/m68k
	ln bin/true bin/sun4
	ln bin/false bin/sun4c
	ln bin/false bin/sun4m
	ln bin/true bin/sparc
elif [ $MINIARCH = "sun3x" ]; then
	ln bin/false bin/sun3
	ln bin/true bin/sun3x
	ln bin/true bin/m68k
	ln bin/false bin/sun4
	ln bin/false bin/sun4c
	ln bin/false bin/sun4m
	ln bin/false bin/sparc
elif [ $MINIARCH = "sun3" ]; then
	ln bin/true bin/sun3
	ln bin/false bin/sun3x
	ln bin/true bin/m68k
	ln bin/false bin/sun4
	ln bin/false bin/sun4c
	ln bin/false bin/sun4m
	ln bin/false bin/sparc
	# KLUDGE for sun3e stupid st tape wont autosense density
	if [ $MINIMEDIATYPE = "tape" ]
	then
		cp $DISTROOT/bin/hostid bin; strip bin/hostid
	fi
else
	echo "unknown MINIARCH \"$MINIARCH\", fix getmunixfs.sh"
	exit 1
fi
#
cp $DISTROOT/usr/bin/ls bin; strip bin/ls
cp $DISTROOT/usr/bin/sh bin; strip bin/sh
cp $DISTROOT/usr/bin/mt bin; strip bin/mt
cp $DISTROOT/usr/bin/sync bin; strip bin/sync
cp $DISTROOT/usr/bin/cat bin; strip bin/cat
cp $DISTROOT/usr/bin/echo bin; strip bin/echo
cp $DISTROOT/usr/bin/rm bin; strip bin/rm
# XXX too big, use cat instead. cp $DISTROOT/usr/bin/cp bin; strip bin/cp
cp $DISTROOT/usr/bin/expr bin; strip bin/expr
# need eject with floppies and cdrom
cp $DISTROOT/usr/bin/eject bin; strip bin/eject
cp $DISTROOT/usr/bin/dd bin; strip bin/dd
cp $DISTROOT/usr/bin/ed bin; strip bin/ed
# XXX too big, used ed instead. cp $DISTROOT/usr/bin/grep bin; strip bin/grep
cp $DISTROOT/usr/bin/stty bin; strip bin/stty
cp $DISTROOT/usr/share/lib/keytables/layout_[0238][0-9a-f] usr/share/lib/keytables
cp $DISTROOT/usr/bin/loadkeys bin; strip bin/loadkeys
if [ $MINIMEDIATYPE = "diskette" ]
then
	# uncompress for floppy miniroot
	cp $DISTROOT/usr/ucb/uncompress usr/ucb; strip usr/ucb/uncompress
fi
if [ $MINIMEDIATYPE = "tape" -o $MINIMEDIATYPE = "cdrom" ]
then
	cp $DISTROOT/usr/bin/mkdir bin; strip bin/mkdir
	cp $DISTROOT/usr/bin/chgrp bin; strip bin/chgrp
	cp $DISTROOT/usr/bin/chmod bin; strip bin/chmod
	cp $DISTROOT/usr/ucb/rsh usr/ucb; strip usr/ucb/rsh
fi
cp $DISTROOT/.profile .profile
echo "loadkeys -e" >> .profile
#
# the profile is special - for ease of use
#	It comes up and asks what it should do - which is usually just
#	roll on the miniroot.
# NOTE: for floppy, miniroot is a compressed image put on
#	two diskettes, see diskette.mk, fdrolloff.c
#	for tape, just a file
#	for cdrom, it's a region of this arch's partition
# see extract.{tape,diskette,cdrom}.sh for the details
#
# some devices need special touches, like floppy needs ejecting of
# diskette "B"
if [ $MINIMEDIATYPE = "diskette" ]
then
	echo "eject /dev/rfd0c" >> .profile
fi
# append generic install-miniroot-off-of-munixfs script
cat ${SUNDISTPATH}/Install.miniroot >> .profile
#
# each media has its own extract script, which is sourced into the shell
# with the variable "disk" set to the disk unit to be loaded with the
# miniroot.  "disk" as is "sd0", "xy1", etc.
# README is to help folks along.
#
# hmmm - blasted PROMS names for "sd" units are much different that
# kernel's, so let the kernel figure everything out.
# to get the info from the kernel, we use a program to read /dev/kmem
# (no kernel in munixfs, so no namelist)
# this is prepended to "extract", even if not used
#
if [ $MINIMEDIATYPE = "cdrom" ]
then
	# HARDCODE miniskip in bytes - miniroot starts at +4Meg 
	#  real Meg, not marketing meg. 4*1024*1024=4194304
	echo "miniskip=4194304" >> extract
	# minicount is size of miniroot in bytes
	minicount=`ls -l ${SUNDISTPATH}/../miniroot.${MINIARCH} |\
		( read perms links owner size junk; echo $size ) `
	echo "minicount=$minicount" >> extract
fi
cat ${SUNDISTPATH}/extract.${MINIMEDIATYPE} >> extract
cp ${SUNDISTPATH}/README.${MINIMEDIATYPE} README
#
# media special programs go here
if [ $MINIMEDIATYPE = "cdrom" ]
then
	cp ${SUNDISTPATH}/fastread.${MINIARCH} etc/fastread
	strip etc/fastread
fi
if [ $MINIMEDIATYPE = "diskette" ]
then
	cp ${SUNDISTPATH}/fdrolloff etc; strip etc/fdrolloff
fi
#
cp $DISTROOT/etc/hosts etc
cp $DISTROOT/etc/ttytab etc
cp $DISTROOT/etc/services etc
# XXX is this neccessary???
cp $DISTROOT/usr/share/lib/zoneinfo/localtime usr/share/lib/zoneinfo

cat >etc/passwd <<EOF
root::0:10::/:/bin/sh
EOF
cat >etc/group <<EOF
wheel:*:0:
kmem:*:2:
operator:*:5:
staff:*:10:
EOF
rm -rf dev; mkdir dev
cp /proto/dev/MAKEDEV dev
chmod +x dev/MAKEDEV
cp /dev/null dev/MAKEDEV.local
cd dev
# fd0 is made "std" on machines that have floppies
# cdroms? - are really used as SCSI disks when installing
./MAKEDEV std
if [ "${MINIARCH}" != "sun4c" -a "${MINIARCH}" != "sun4m" ]
then
	./MAKEDEV xy0 xy1 xy2 xy3 xd0 xd1 xd2 xd3 xd4 xd5 xd6 xd7
	rm xy*
	./MAKEDEV xd8 xd9 xd10 xd11 xd12 xd13 xd14 xd15 # all disks
	rm xd*
fi
if [ "${MINIARCH}" = "sun4" -o "${MINIARCH}" = "sun4m" ]
then
        ./MAKEDEV id000 id001 id002 id003 id004 id005 id006 id007
	rm id0* rid0*[adefgh]
        ./MAKEDEV id010 id011 id012 id013 id014 id015 id016 id017
	rm id0* rid0*[adefgh]
        ./MAKEDEV id020 id021 id022 id023 id024 id025 id026 id027
	rm id0* rid0*[adefgh]
        ./MAKEDEV id030 id031 id032 id033 id034 id035 id036 id037
	rm id0* rid0*[adefgh]
	./MAKEDEV id040 id041 id042 id043 id044 id045 id046 id047 # ipi disks
	rm id0* rid0*[adefgh]
fi
mknod rd0a b 13 0	# so bootargs can be read from ramdisk
cd ..
sync
