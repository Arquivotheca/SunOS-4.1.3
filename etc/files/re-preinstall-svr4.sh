#!/bin/sh
#
# %Z%%M% %I% %E% SMI
# Copyright 1992 Sun Microsystems Inc.
#
# re-preinstall-svr4.sh - phase 1 of the 4.1.3 plan_4 preinstall stub
#	that will find and load the 5.1 preinstall stub.
#
# ensures in single user mode
# tries to find the network, then tries to find a local CD
# if it finds one of them, it mounts them on the stub directory,
#

PATH=/etc:/sbin:/usr/bin:/usr/etc:/usr/ucb:/usr/etc/install
export PATH

# export these things, so .stub.4x.part2 can get them too
IDIR="/etc/.install.d"	# initial place, hidden things go here
export IDIR
STUB="/.stub"
export STUB

CDROM_DEV="/dev/sr0"

myname=re-preinstall-svr4

#
# FUNCTIONS --------------------------------------------------------------
#

#
# cleanup reverses anything that was done (before the final approval)
# and exits with a non-zero value
#
DIDMOUNT=""
cleanup() {

	if [ "${DIDMOUNT}" ]; then
		${DIDMOUNT} 2> /dev/null
	fi
	if [ "${MADESTUBMNT}" ]; then
		${MADESTUBMNT}
	fi
	if [ "${MADESTUB}" ]; then
		${MADESTUB}
	fi
	exit 1
	# =====
}

#
# MAIN --------------------------------------------------------------
#

# tests for "mostly" single user mode
#	ie less than a few processes
# checks for not diskless
# prints a waiting for.. message
# probes for remote media with bpgetfile program
# if no remote media,
#	probe for a local CDROM
# if no media found,
#	 print a message
#	 give up
# mkdir /.stub/mnt
# try to mount the media image
# if error mounting media,
#	print a message
#	rmdir /.stub/mnt if had to create
#	give up
# ? try to find localization file on media
# probe media for readable, non-zero length stub.cpio.4x
# if not found,
#	print a message (localized?)
#	unmount media		
#	rmdir /.stub/mnt if had to create
#	give up
# print a warning (?localized?) about what will happen
# require "1" or "2" to continue, exit
#	if 1, continue
#	if 2,
#		unmount media		
#		rmdir /.stub/mnt if had to create
#		exit
# --- now we can really modify things ---
# touch /etc/nologin
# move /sbin/sh aside to /stub, and set PATH to include /.stub
# move /etc/.install.d/*  to /.stub, and set PATH to include /.stub
# exec copied/moved /sbin/sh KVMDIR/.stub.4x.part2

KARCH=`arch -k`
if [ ! "${KARCH}" ]; then
	echo "${myname}: cannot find karch with \"arch -k\""
	cleanup
fi

# we'll need FILES to cleanup after the cpio of the stub
# so we check that these exist
FILES="/usr/bin/ln /sbin/sh /usr/etc/install/xxx"
# these files are smaller than FILESIZE KBytes
FILESIZE=200
for i in ${FILES} ;
do
	if [ ! -f $i ]; then
		echo "${myname}: cannot find \"${i}\", abandoning upgrade"
		cleanup
	fi
done
# plus one we just use in place
if [ ! -f /usr/etc/install/bpgetfile ]; then
	echo "${myname}: cannot find \"/usr/etc/install/bpgetfile\", abandoning upgrade"
	cleanup
fi

# test for single user mode
nproc=`ps axl | wc -l`
if [ ${nproc} -gt 8 ]; then
        echo "${myname}: too many processes, please run me in single user"
        echo "    \"kill -TERM 1\" or \"reboot -s\" will get you there"
        cleanup
fi

# test for NOT a diskless (root) machine
root=`df / | ( read junk; read dev junk; echo ${dev} )` 
if [ ! -b "${root}" ] ; then
        echo "${myname}: this upgrade is not for diskless machines"
        cleanup
fi

# beforehand, check for existance of /.stub/mnt, ok?
if [ ! -d ${STUB} ]; then
	mkdir ${STUB}
	MADESTUB="rmdir ${STUB}"
else
	MADESTUB=""
fi
if [ ! -d ${STUB}/mnt ]; then
	mkdir ${STUB}/mnt
	MADESTUBMNT="rmdir ${STUB}/mnt"
else
	MADESTUBMNT=""
fi

# note: in rc.boot, hostconfig has already run, so network is alive if there.

#
# try to invoke bpgetfile for install keyword(s)
# to see if there is a network version of the CD image
#
echo "probing for remote media, this will take a minute"
#
LOCATION=""
BPGETF=`bpgetfile -retries 1 install | ( read it; echo $it)`
if [ "${BPGETF}" ] ; then
	set ${BPGETF}
	# got them ok, mount the CD image
	mount -r -t nfs ${2}:${3} ${STUB}/mnt
	if [ $? -ne 0 ]; then
		echo "${myname}: Failed mount of ${2}:${3} on ${STUB}/mnt"
		cleanup
		# =====
	fi
	DIDMOUNT="umount ${STUB}/mnt"
	MEDIA="${2}:${3}"
	LOCATION="remote"
# else see if a local CD is here
elif [ -b ${CDROM_DEV} ] ; then
	# try to mount it (HSFS)
	# if already mounted - bail out
	cdmounted=`mount | grep "${CDROM_DEV}" `
	if [ "${cdmounted}" ] ; then
		echo "${myname}: ${CDROM_DEV} is already mounted"
		echo "    please unmount it"
		cleanup
		# ====
	fi
	mount -t hsfs -r ${CDROM_DEV} ${STUB}/mnt
	if [ $? -eq 0 ]; then
		DIDMOUNT="umount ${STUB}/mnt"
		MEDIA=${CDROM_DEV}
		LOCATION="local"
	fi
fi
if [ ! "${LOCATION}" ] ; then
        echo "${myname}: did not find either remote media or local install CD"
        cleanup
        # =====
fi

#
# try to find the path to the correct root on the media.
# it must be 2.1 or greater
# the name will be: <mnt>/export/exec/kvm/<mumble>.<MYKARCH>.<something_vers>
# if not found,	print a message (localized?), cleanup and exit
KVMPATH="export/exec/kvm"
# snarf the (only? first?) kvm that matches our karch (_should_ only be one...)
KVMDIR=`echo ${STUB}/mnt/${KVMPATH}/*.${KARCH}.* | ( read a b ; echo $a)`
if [ ! -d "${KVMDIR}" ]; then
	echo "${myname}: Invalid media, the directory ${KVMDIR} was NOT found on ${MEDIA}"
	cleanup
	# =====
fi
export KVMDIR		# pass along to phase 2

# check for a "magic" hidden files that say "we are stub capable media"
if [ ! -f "${KVMDIR}/.stub.4x.check" -o ! -f "${KVMDIR}/.stub.4x.part2" ]; then
	echo "${myname}: Invalid media: ${MEDIA}"
	cleanup
	# =====
fi

# need to find a valid bootblk too
# it will be in <mnt>/export/exec/<mach|arch>.foo/lib/fs/ufs/bootblk
ARCH=`mach`
BOOTBLK=`echo ${STUB}/mnt/${KVMPATH}/../${ARCH}.* | ( read a b ; echo $a)`
BOOTBLK="${BOOTBLK}/lib/fs/ufs/bootblk"
if [ ! "${BOOTBLK}" -o ! -f "${BOOTBLK}" ]; then
	# try arch
	ARCH=`arch`
	BOOTBLK=`echo ${STUB}/mnt/${KVMPATH}/../${ARCH}.* | ( read a b ; echo $a)`
	BOOTBLK="${BOOTBLK}/lib/fs/ufs/bootblk"
fi

if [ ! "${BOOTBLK}" -o ! -f "${BOOTBLK}" ]; then
	echo "${myname}: BOOTBLK not found: ${BOOTBLK}"
	exit 1
fi
export BOOTBLK

# now see if there is enough space in the root to load the stub system
ROOTSIZE=`df / | ( read junk; read dsk kb junk; echo ${kb} )` 

if [ ! "${ROOTSIZE}" ] ; then
	echo "${myname}: cannot figure out root size: ${ROOTSIZE}"
	cleanup
	# =====
fi

free=`df / | ( read junk; read dsk kb use avail junk; echo $avail )`
if [ ! "${free}" -o "${free}" -lt ${FILESIZE} ]; then
	echo "${myname}: not enough free space on root, need ${FILESIZE} KB"
	cleanup
	# =====
fi

ROOTSIZE=`expr ${ROOTSIZE} - ${FILESIZE}`
if [ ! "${ROOTSIZE}" ] ; then
	echo "${myname}: cannot calculate root size: ${ROOTSIZE}"
	cleanup
	# =====
fi
export ROOTSIZE

#
# because we don't know what policy, etc. will always be needed,
# the check is _sourced_ from the media.
# (and its existance was previously verified)
# it either falls thru or invokes the function "cleanup" which exits
#
. ${KVMDIR}/.stub.4x.check

# print a warning (?localized?) about what will happen
# require "1" or "2" to continue, exit
#	if 1, press on
#	if 2, unmount media,rmdir /.stub/mnt if had to create, exit
while :
do
	# LOCALIZE ? XXX
	echo ""
	echo "You have invoked the upgrade path to SunOS 5.X"
	echo "This process is irreversible if you continue."
	echo "All data on your disk may be lost, and they may be"
	echo "repartitioned, depending on the install customizations"
	echo "the system administrators have specified."
	echo ""
	echo "You should have backed up all data and files from this machine"
	echo "If not, stop this process and do so."
	echo ""
	echo "Do you wish to continue?"
	echo "  1 - yes, continue with upgrade"
	echo "  2 - no, stop this process"
	echo -n "Enter a 1 or 2: "
	# end LOCALIZE
	read ans
	case $ans in
	1)
		# they said yes
		break;
		;;
	2) 
		# print a nice message
		# LOCALIZE
		echo "exiting this process"
		cleanup
		# =====
		;;
	*)
		# unknown answer, back to while 1
		# LOCALIZE
		echo "unknown answer \"${ans}\", expecting \"1\" or \"2\""
		;;
	esac
done

# --- now we can really modify things ---
# move a few needed things, and hide /usr, unmount everything not needed,
# remove evernthing else on /, then exec /sbin/sh /.stub/re-pre-svr4.part2

trap "" 1 2 3 15

touch /etc/nologin
for i in "${FILES}" ; do
	cp $i ${STUB}
done

# now invoke phase 2 for the tricky stuff
cd /
exec ${STUB}/sh ${KVMDIR}/.stub.4x.part2

