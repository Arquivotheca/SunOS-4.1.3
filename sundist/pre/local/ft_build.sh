#!/bin/sh
# %Z%%M% %I% %E% SMI
#
# This script is to be run in FT. It will:
#	
#	1) re-install and check root
#	2) check usr
#
# A disk processed by this script is OK to ship provided it is
# NEVER written to after this script has passed.
#
# Currently this script supports campus, campusB, and hydra.
#
# This script requires:
#
#       1) disk_build and all the prerequisites of disk_build
#

ARCH=`arch -k`
CMDNAME=`basename $0`

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

#
# Install and verify the disk, both root and usr partitions
#
disk_build -r
[ $? -ne 0 ] && Failmsg && cleanup && exit 1

#
# success!
#
echo ""
echo "***** Exiting $CMDNAME Version %I%  `date` *****"
echo ""

