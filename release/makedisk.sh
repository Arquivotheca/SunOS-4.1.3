#!/bin/sh
# %Z%%M% %I% %E% SMI
#
# preinstall a disk with a 4.1 filesystem
#
# Assume:
# 	system booted off nfs
#	disk sd0 is labelled and formatted
#	prototype file system in /proto
#
#Usage="$0 [-r rootdev] [-u usrdev] [-f] \
#	[-p proto_dir] [-s source_path] [-h hostname] [-d domainname]"
#

ROOT=/dev/sd0a
USR=/dev/sd0g
PROTO=/proto
SRC=justsayno:/rel/proto
FIRSTDISK=false
PRE=/pre
PREUSR=/pre/usr
ARCH=`arch -k`
HOSTNAME=`hostname`
DOMAINNAME=`domainname`

set -- `getopt r:u:fp:s:h:d: $*`
if [ $? != 0 ]; then
	echo Usage: $Usage >&2
	exit 1
fi
for i in $*; do
	case $i in
	-r)	ROOT=$2; shift 2;;
	-u)	USR=$2; shift 2;;
	-p)	PROTO=$2; shift 2;;
	-s)	SRC=$2; shift 2;;
        -h)     HOSTNAME=$2; shift 2;;
        -d)     DOMAINNAME=$2; shift 2;;
	-f)	FIRSTDISK=true; shift;;
	--)	shift; break;;
	esac
done

echo "This program will scrog partitions $ROOT and $USR."
echo -n "Is that all right with you? "
read yesno
case ${yesno:="yes"} in
	[yY]*)	true;;
	*)	exit 1;;
esac

RAWROOT=`echo $ROOT | awk -F/ '{printf("/dev/r%s", $3)}'`
RAWUSR=`echo $USR | awk -F/ '{printf("/dev/r%s", $3)}'`

#
# you must be root
#

if [ "`whoami`x" != "root"x ]; then
   echo "You must be root to do $0!"
   exit 1
fi

#
# If /proto's not mounted, try.
#
echo "Checking /proto..."
/etc/mount | grep -s $PROTO 
if [ $? -ne 0 ]; then
	if [ ! -d $PROTO ]; then
		mkdir $PROTO
	fi
	/etc/mount ${SRC} $PROTO
	if [ $? -ne 0 ]; then
		echo "$PROTO not mounted?"
		exit 1
	fi
fi

#
# Construct exclude list
#
echo "./crypt*" > /tmp/exclude$$

#
# find preinstall devices and get them ready
#
echo "Preparing file systems..."
for i in $ROOT $USR; do
	if /etc/mount | grep -s $i; then
		echo "$i is a mounted file system!"
		exit 1
	fi
done
for i in $RAWROOT $RAWUSR; do
	newfs $i 
	if [ $? -ne 0 ]; then
		echo "newfs failed on $i. Is it formatted?"
		exit 1
	fi
done
test -d $PRE || mkdir $PRE
mount $ROOT $PRE
mkdir $PREUSR
mount $USR $PREUSR
chmod -R g+s $PRE && chgrp -R staff $PRE

#
# install everything, for whatever we have defined everything to be
#
echo "Installing release...(this takes quite a while)..."
cd $PROTO
tar cfX - /tmp/exclude$$ . | (cd $PRE; tar xfBp -)
rm /tmp/exclude$$
rm -rf $PRE/crypt*

#
# clean up the leftovers
#
echo "Cleaning up..."
#
cd $PREUSR/stand
cp kadb vmunix $PRE
cp boot.$ARCH $PRE/boot
#
cd $PREUSR/boot
cp ifconfig init intr mount $PRE/sbin
cd $PREUSR/bin
cp sh hostname $PRE/sbin
#
cd $PRE/dev
MAKEDEV std pty0 pty1 pty2 win0 win1 cgthree0 cgsix0 bwtwo0

if $FIRSTDISK; then
	cat << EOF > $PRE/etc/fstab
/dev/sd0a	/		4.2 rw          1 1
/dev/sd0g	/usr		4.2 rw          1 1
EOF
else
	cat << EOF > $PRE/etc/fstab
$ROOT	/		4.2 rw          1 1
$USR	/usr		4.2 rw          1 1
EOF
fi

#
# 4.1 needs these files
#
echo $HOSTNAME > $PRE/etc/hostname.le0
echo $DOMAINNAME > $PRE/etc/defaultdomain

#
# Activate unpack.
#

touch $PRE/etc/.UNCONFIGURED
mkdir -p $PREUSR/export/home

cd $PREUSR/mdec
installboot -v $PRE/boot bootsd $RAWROOT
cd /
umount $PREUSR
umount $PRE
