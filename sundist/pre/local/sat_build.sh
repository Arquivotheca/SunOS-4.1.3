#!/bin/sh
# %Z%%M% %I% %E% SMI
#
# This script is to be run in SAT. It will:
#	
#	1) completely install and check root and usr
#	2) customize root to boot diskful and netless
#
# NEVER ship a disk that has been processed by this script
# until it has successfully passed ftbuild.
#
# Currently this script supports campus, campusB, and hydra.
#
# This script requires:
#
#       1) disk_build and all the prerequisites of disk_build
#

ARCH=`arch -k`
HOSTID=`hostid | colrm 3`
CMDNAME=`basename $0`
MNTROOT=/mntroot
UNCONFIGURED=etc/.UNCONFIGURED
UNPACK=etc/.UNPACK
PATH=$PATH:/etc:/usr/etc; export PATH

cleanup() {
	umount $MNTROOT > /dev/null 2>&1
}
Failmsg() {
echo "******************"
echo "*                *"
echo "*    FAILURE!    *"
echo "*                *"
echo "******************"
}
trap 'echo $0 ABORTED.; cleanup; exit 1' 1 2 15

#
# you must be root to run this script
#
if [ "`whoami`" != root ]; then
   echo "You must be root to do $CMDNAME!"
   exit 1
fi

#========================================================================
#
# Workaround for 4.1 FCS: install the root using "rootfs_mfg"
# and exit.
#
disk_build -R rootfs_mfg
[ $? -ne 0 ] && Failmsg && cleanup && exit 1
echo ""
echo "***** Exiting $CMDNAME Version %I%  `date` *****"
echo ""
exit 0
#
# End workaround.
#
#========================================================================

#
# Install and verify the disk, both root and usr partitions
#
disk_build -f
[ $? -ne 0 ] && Failmsg && cleanup && exit 1

#
# Mount the root partition in order to modify for diskful boot
# and to install sundiag.
#
case $HOSTID in
        42)  DISK="sd6"; MACHINE="hydra";;
        51)  DISK="sd0"; MACHINE="campus";;
        52)  DISK="sd0"; MACHINE="phoenix";;
        53)  DISK="sd0"; MACHINE="campusB";; 
        55)  DISK="sd0"; MACHINE="calvin";; 
        *)   echo "$CMDNAME:  Host Id $HOSTID not supported"; exit 1;;
esac

ROOT="/dev/${DISK}a"
test -d $MNTROOT || mkdir $MNTROOT
echo "Mounting $ROOT on $MNTROOT ..."
while :
do
	mount $ROOT $MNTROOT 
	if [ $? != 0 ]; then
		echo "Unable to mount $MNTROOT !"
		cleanup 
		exit 1
	fi
	break;
done

#
# For diskful boot: for now, just disable yp and sys-config.
#
echo "Editing rc.local on $MNTROOT ..."
rm -f $MNTROOT/$UNCONFIGURED $MNTROOT/$UNPACK
ed -s $MNTROOT/etc/rc.local << EOF
/ypbind/s/^/false \&\& /
w
q
EOF

#
# Unmount and clean up.
#
echo "Unmounting $ROOT on $MNTROOT ..."
umount $MNTROOT
rmdir $MNTROOT > /dev/null 2>&1

echo ""
echo "***** Exiting $CMDNAME Version %I%  `date` *****"
echo ""
