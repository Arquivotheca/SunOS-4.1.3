#
# @(#)re-pre-svr4.part2.sh 1.1 92/07/30 SMI
# Copyright 1992 Sun Microsystems Inc.
#
# re-pre-svr4.part2.sh - phase 2 of the 4.1.3 plan_4 preinstall stub.
#	run by phase 1, don't you even dare try to run this by itself!
#	This will cause every file to be removed from your root disk.
#
# cleans up the root, then cpio's the 5.X install root (previously
# mounted by phase1) onto the root, then reboots.
#
# INPUTs: (from environment)
#	STUB - path of stub dir
#	KVMDIR - path of the root to cpio copy
#	BOOTBLK - path to the bootblock off the media
#
# things needed for cpio, sync, reboot are moved/linked to /.stub
# LD_LIBRARY_PATH is set to ${STUB}/usr/lib

myname=re-pre-svr4.part2

if [ ! "${STUB}" -o ! "${KVMDIR}" -o ! "${BOOTBLK}" ]; then
	echo "${myname}: STUB undefined, bailing out"
	exit 1
fi

ROOTDISK=`df / | ( read junk ; read disk junk; echo $disk )`
# we want to reboot off _exactly_ the root disk (if we can)
if [ -x /usr/kvm/unixname2bootname ]; then
	REBOOT=`/usr/kvm/unixname2bootname ${ROOTDISK}`
else
	REBOOT=""	# sorry, poor old sun4
fi

# unmount all but root, (kvm), /usr and the media
mount | while read mount on path type fstype stuff ; do
	case $path in
	/|/usr|"${STUB}/mnt")
		# nothing, leave alone
		continue
		;;
	*)	umount ${path}
		;;
	esac
done

# we cannot move "/usr" to a hidden place because ld.so can only come
# from /usr/lib hardcoded path.  So, we play some games to use /usr
# until the very last moment
 
# grab the media "/usr" link now, while we have the tools to do so
USRLNK=`/usr/bin/ls -l "${KVMDIR}/usr" | \
	( read mod nlks own sz mon day time name pt lname; echo $lname )`

# clean up the root partition of everything that isn't needed
#  things that show
RM1=`/usr/bin/ls -ad /* | egrep -v "^/tmp|^/usr|^/dev"`
/usr/bin/rm -rf ${RM1}
/usr/bin/rm -rf /tmp/*

#  things that are hidden, but NOT "/." or "/.." or the stub
RM2=`/usr/bin/ls -ad /.[a-zA-Z0-9]* | /usr/bin/grep -v "\</.stub\>"`
/usr/bin/rm -rf ${RM2}

# install the new bootblock, before we trash /dev/zero
/usr/bin/dd if=${ROOTDISK} of=/tmp/label$$ bs=1b count=1 2>/dev/null
/usr/bin/cat /tmp/label$$ $BOOTBLK | \
	/usr/bin/dd of=$ROOTDISK bs=1b count=16 conv=sync 2>/dev/null
/usr/bin/rm -f /tmp/label$$
echo "" > /.PREINSTALL		# mark as "stub installed"
/usr/bin/sync

# ok, now snarf the 5.X stub off the media and reboot it
echo "copying stub, this will take 2-4 minutes"
cd ${KVMDIR}
/usr/bin/cpio -pdum / < ${KVMDIR}/.stub.4x.cpio


${STUB}/xxx umount /usr
${STUB}/xxx rm /usr
${STUB}/ln -s ${USRLNK} /usr
${STUB}/xxx reboot "${REBOOT}"

