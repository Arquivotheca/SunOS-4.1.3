#! /bin/sh
# 
#       @(#)buildmini.sh 4.29 89/09/19 SMI
#
# usage: buildmini [arch]
#
# Used the following environment variables, if they are not set
# defaults are chosen:
#
#	name		use			default
#
#	MINIARCH	architecture		`arch -k`
#	NBPI		number of bytes/inode	passed from Makefile
#	MINISIZE	fs size in sectors	derived from dkinfo
#	MINIDEV		fs block device		derived from fstab
#	MINIMNT		fs mount point		./minirootmnt
#	MINIGET		sh script to get files	./bin/getmini
#	MININAME	name of dd'ed file	./miniroot.MINIARCH

CWD=`pwd`

if [ $# = 1 ]; then
	MINIARCH=$1
fi

if [ $MINIARCH"x" = "x" ]; then
	MINIARCH=`arch -k`
fi

if [ $MINIMNT"x" = "x" ]; then
	MINIMNT=/miniroot
fi

if [ ! -d $MINIMNT ]; then
	mkdir $MINIMNT
fi

if [ $MINIDEV"x" = "x" ]; then
	MINIDEV=`grep $MINIMNT /etc/fstab | sed -e 's,[ 	].*,,'`
	export MINIDEV
fi

if [ $MINIDEV"x" = "x" ]; then
	echo "$0: can't find device for mount point $MINIMNT"
	exit 1
fi

BDEV=`basename $MINIDEV`
RDEV=/dev/r$BDEV

if [ $MINIGET"x" = "x" ]; then
	MINIGET=$CWD/bin/getmini
fi

if [ $MININAME"x" = "x" ]; then
	MININAME=miniroot.$MINIARCH
fi

if [ $MINISIZE"x" = "x" ]; then
	MINISIZE=`dkinfo $BDEV | grep " sectors " | sed -e 's,[ 	].*,,'`
fi

echo building $MININAME using $MINIGET on $MINIDEV with $MINISIZE sectors

umount $MINIMNT
# if partition is unmounted (so mkfs will work), then...
if mount 2>&1 | grep "/dev/${BDEV}" > /dev/null;
then
	:	# grep found it mounted, leave it alone
else
	# ...zorch the intended miniroot - so it will compress nicely
	# Warning: this dd asumes that the sector size == 512 bytes!
	echo "zeroing miniroot so it will compress nicely"
        # skip over the disk label
        dd if=/dev/zero of=$RDEV bs=1b seek=1 count=15
        dd if=/dev/zero of=$RDEV bs=8k seek=1 count=`expr $MINISIZE / 16 - 1`
fi
mkfs $RDEV $MINISIZE 32 8 8192 1024 16 10 60 $NBPI
fsck -p $RDEV
tunefs -m 0 $RDEV
mount $MINIMNT
(cd $MINIMNT; sh -x $MINIGET)
sync
umount $MINIMNT
fsck -p $RDEV
df $RDEV
# Warning: this dd asumes that the sector size == 512 bytes!
echo dd if=$RDEV of=$MININAME bs=8k count=`expr '(' $MINISIZE + 15 ')' / 16`
dd if=$RDEV of=$MININAME bs=8k count=`expr '(' $MINISIZE + 15 ')' / 16`

echo $MININAME done
