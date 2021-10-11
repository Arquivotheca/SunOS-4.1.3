#!/bin/sh
# %Z%%M% %I% %E% SMI
#
# This script will preinstall a disk by with dd images from
# from a server.  Currently it supports campus, campusB, and hydra.
# This script will copy images from a server to net-booted clients.
# It is intended to support disks with the following labels:
#	Quantum_ProDrive_105S
#	SUN????
#
# Before executing this script, we assume:
#
#       1) system booted off nfs
#
# 	2) you should be root
#
#	3) disk to be preinstalled (ie: sd0/sd6) is formatted
#
#       4) dd golden images are on a server in directory:
#
#                    /home/RELEASE/ARCH/DISKTYPE
#
#          example:  /home/4.0.3/sun4c/Quantum_ProDrive_105S
#
#          and it contains the following files:
#
#                  rootfs    - dd image of root file system (a partition)
#                  userfs    - dd image of usr file system  (g partition)
#                  checksums - checksums of rootfs, userfs,  
#				obtained from the crc program 
#	Note there is no "homefs".  This is created with newfs.
#
#          The server's /home partition should be mounted and exported 
#          as read only on the server.
#
#          The crc program (calculates the checksum) should be 
#          installed in /home/tools/bin.  There should be
#          one each for sun3 and sun4 (i.e. crc.sun4 and crc.sun3).
#

CMDNAME=`basename $0`
Usage='
echo "Usage: $CMDNAME [ -rvf ] [ -s server ] [ -g golden ] [ -R rootfs ]"
echo "No options:  preinstall & verify root and usr partitions"
echo "-r           preinstall root partition only; verify root and usr"
echo "-v           verify checksums of root and usr (do not do preinstall)"
echo "-s server    server containing preinstall files"
echo "                (a default server name is formed from the first"
echo "                two characters of the hostname with \"-atn\" appended)"
echo "-g golden    path name of preinstalled files (default \"/home\")"
echo "-f           modify fstab so usr is mounted read-only"
echo "-R rootfs    rootfs image file (default \"rootfs\")"
'

#
# you must be root to run this script
#
if [ "`whoami`x" != "root"x ]; then
   echo "You must be root to do $CMDNAME!"
   exit 1
fi

#
# get command line parameters 
#
install_root=1
install_user=1
roption=""
voption=""
foption=""
SERVER="`hostname | colrm 3`-atn"
GOLDEN=/home
ROOTFS=rootfs

while [ "$1"x != "x" ]; do
        case $1 in
        -r)     roption=1; install_user=""; shift;;
        -v)     voption=1; install_root=""; install_user="";  shift;;
        -s)     SERVER=$2; 
		if [ ! "$SERVER" ]; then 
		     eval "$Usage"; exit 1;
		fi 
		shift 2
		;;
        -g)     GOLDEN=$2; 
		if [ ! "$GOLDEN" ]; then 
		     eval "$Usage"; exit 1;
		fi 
		shift 2
		;;
        -R)     ROOTFS=$2; 
		if [ ! "$ROOTFS" ]; then 
		     eval "$Usage"; exit 1;
		fi 
		shift 2
		;;
        -f)     foption=1;     shift;;
        *)      eval "$Usage"; exit 1;;
        esac
done

#
# -r option not compatible with -v option
#
[ "$voption" ] && [ "$roption" ] && 
	echo "-r option not compatible with -v option" &&
	exit 1

#
# server name must be present
#
[ ! "$SERVER" ] && echo "You must enter a server name!" && exit 1

#
# yes/no responses
#
get_yn='(while read yn; do
        case "$yn" in
                [Yy]* ) exit 0;;
                [Nn]* ) exit 1;;
                    * ) echo -n "Please answer y or n:  ";;
        esac
done)'

Failmsg='
echo "******************"
echo "*                *"
echo "*    FAILURE!    *"
echo "*                *"
echo "******************"
'

cleanup='$umount $MNTROOT > /dev/null 2>&1; rm -f $tmp/*$$;'
trap 'echo $0 ABORTED.; eval "$cleanup"; exit 1' 1 2 15

#
# Working variables--these must be set *after* evaluating command line
# arguments
#
RELEASE=4.1
BIN=${GOLDEN}/tools/bin
ARCH=`arch -k`
HOSTID=`hostid | colrm 3`
umount=/etc/umount
mount=/etc/mount
USERFS=userfs
CHECKSUM=checksum
DISKTYPE=
MACHINE=
MNTROOT=/mntroot
BLOCKSIZE=1024k
crc=${BIN}/crc.`arch`
tmp=/tmp

#
# get disk to preinstall
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
USER="/dev/${DISK}g"
RAWROOT=`echo $ROOT | awk -F/ '{printf("/dev/r%s", $3)}'`
RAWUSER=`echo $USER | awk -F/ '{printf("/dev/r%s", $3)}'`

#
# Prompt user to continue or exit
#
echo ""
echo "***** Starting $CMDNAME Version %I%  `date` *****"
echo ""
echo "Machine is a $MACHINE; arch is $ARCH; release is $RELEASE."
echo ""
echo "Hostname is `hostname`; Server is $SERVER."
echo ""
echo "You have selected the following options: "
echo ""
[ "$install_root" ] && \
            echo "  --> Preinstall root partition $ROOT"
[ "$install_user" ] && \
            echo "  --> Preinstall usr partition $USER" 
	    echo "  --> Build home partition if DISKSIZE > 130 MB."
[ "$voption" ]  && \
            echo "  --> Verify checksums only (do not do preinstall)"
[ "$foption" ]   && \
            echo "  --> Modify fstab so usr is mounted read-only" 

echo ""
echo "Probing for disk $DISK ... "
#
# make sure disk exists with format; get disk model
#
echo '0\nquit' | format > $tmp/disktmp$$ 2> /dev/null
# toss 1st 4 lines, toss last line, toss big whitespace
# toss everything from "cyl" on over on second lines of output 
# from format(8)
# 2x w in case little space remains, eh sun3e?
cat << XXX | ed -s $tmp/disktmp$$
1,4d
\$d
1,\$s/[         ]*//
1,\$s/ cyl [0-9][0-9]*.*\$//
w
w
q
XXX
#
# make sure there are some disks out there
#
[ ! -s $tmp/disktmp$$ ] && \
echo "No disks found! Must have disks to run $CMDNAME" && eval "$cleanup" \
     && exit 1
#
# the ed step should leave you with something like...
# 0. sd0 at esp0 slave 24
#    sd0: <Quantum ProDrive 105S
# 1. sd1 at esp0 slave 8
#    sd1: <Quantum ProDrive 105S
#
#	- OR -
#
# 0. sd0 at esp0 slave 24
#    sd0: <SUN0207 
# 1. sd1 at esp0 slave 8
#    sd1: <SUN0207
#
# these lines are joined and parsed with shell read...
#
cat /dev/null > $tmp/diskhere$$
cat /dev/null > $tmp/disktype$$
while read number name w1 w2 w3 w4
do
        read namecolon maker model
        if [ $name = $DISK ]; then
   		echo $number | tr -d '.' > $tmp/number$$

#		Now see if we even have ANY model

		if [ -n "${maker}" -a -z "${model}" ]; then
	   		echo "${DISK} is a ${maker}>"
			echo `echo "$maker" | colrm 1 1 ` > $tmp/disktype$$
		else
	   		echo "${DISK} is a ${maker} ${model}>"
           		echo `echo -n "${maker}_" | colrm 1 1; \
			      echo ${model} | tr ' ' '_';` > $tmp/disktype$$
		fi
        fi 
#	If $model is empty, this should still work
        echo $name "${maker} ${model}>" $w1 $w2 $w3 $w4 >> $tmp/diskhere$$

done < $tmp/disktmp$$
# 
# if DISKTYPE is empty, then disk to preinstall not found
# 
DISKTYPE=`cat $tmp/disktype$$`
if [ ! "$DISKTYPE" ]; then
        echo "Disk $DISK not found!" 
        echo "Disks found are:" 
        cat $tmp/diskhere$$
        eval "$cleanup" && exit 1 
fi 
rm -f $tmp/disktype$$ $tmp/diskhere$$ $tmp/disktmp$$

#
# build path for mounting golden files
#
DIR_PATH="${GOLDEN}/${RELEASE}/${ARCH}/${DISKTYPE}"

#
# make sure rootfs, userfs, checksum and crc files exist before proceeding
#
echo "Checking for existence of files ..."
[ "$install_root" ] && [ ! -f $DIR_PATH/$ROOTFS ] \
	&& echo "Unable to locate $DIR_PATH/$ROOTFS !" \
	&& eval "$cleanup" && exit 1

[ "$install_root" ] && [ ! -f $DIR_PATH/${CHECKSUM}.$ROOTFS ] \
	&& echo "Unable to locate  $DIR_PATH/${CHECKSUM}.$ROOTFS !" \
	&& eval "$cleanup"  && exit 1

[ "$install_user" ] &&  [ ! -f $DIR_PATH/$USERFS ]  \
	&& echo "Unable to locate $DIR_PATH/$USERFS !" \
	&& eval "$cleanup" && exit 1

[ "$install_user" ] && [ ! -f $DIR_PATH/${CHECKSUM}.$USERFS ] \
	&& echo "Unable to locate  $DIR_PATH/${CHECKSUM}.$USERFS !" \
	&& eval "$cleanup"  && exit 1

[ ! -f $crc ] \
	&& echo "Unable to locate $crc !" && eval "$cleanup" && exit 1 

#
# make sure there are no mounted file systems on local disk before preinstalling
#
echo "Checking for mounted file systems ..."
if [ "$install_root" ] || [ "$install_user" ]; then
	if $mount | grep -s /dev/${DISK}; then
		echo ""
 		echo "${DISK} has mounted file system(s)!" 
		$mount | grep /dev/${DISK} 
		echo "Please unmount ${DISK} before doing $CMDNAME."
		eval "$cleanup" && exit 1
	fi
fi

#
# Ready to start preinstalling
#

if [ "$install_root" ]; then
	echo ""
	echo "Preinstalling $ROOT with $DIR_PATH/$ROOTFS ..."
	dd bs=$BLOCKSIZE if=$DIR_PATH/$ROOTFS of=$RAWROOT 
        [ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" \
		&& exit 1 

	echo "Relabeling disk $DISK ..."
	echo "disk"        > $tmp/cmdfile$$
	cat $tmp/number$$ >> $tmp/cmdfile$$
	echo "label"      >> $tmp/cmdfile$$
	echo "quit"       >> $tmp/cmdfile$$
	format -f $tmp/cmdfile$$ 1> /dev/null  
	[ $? != 0 ] && eval "$Failmsg" && eval "$cleanup" && exit 1
	rm -f $tmp/cmdfile$$ $tmp/number$$
fi

echo "Verifying $ROOT checksum ..."

CHECK=`cat $DIR_PATH/${CHECKSUM}.$ROOTFS`
[ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" && exit 1
echo "Checksum of $ROOTFS is    $CHECK"

CRC=`$crc $RAWROOT`
[ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" && exit 1 
echo "Checksum of $ROOT is $CRC"

if [ $CHECK != $CRC ]; then
	echo "Checksum miscompare."
	eval "$Failmsg"; eval "$cleanup"; exit 1
else
	echo ""
	echo "Checksum matches; $ROOT PASSES!"
fi

if [ "$install_user" ]; then
        echo ""
        echo "Preinstalling $USER with $DIR_PATH/$USERFS ..."
        dd bs=$BLOCKSIZE if=$DIR_PATH/$USERFS of=$RAWUSER 
        [ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" \
                && exit 1
	
#
#	Now we create an h iff we have room for one
#	Iff our disksize (as indicated by the generic name) is > 130 mb
#	then we put a /home on it
#
	if [ "`echo ${DISKTYPE} | colrm 4`" = "SUN" ]; then
		if [ `echo ${DISKTYPE} | colrm 1 3` -gt 130 ]; then
			echo "Creating /home parition..."
			newfs /dev/rsd0h > /dev/null
        		[ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" \
                		&& exit 1
		fi
	fi
fi
 
echo "Verifying $USER checksum ..."

CHECK=`cat $DIR_PATH/${CHECKSUM}.$USERFS`
[ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" && exit 1
echo "Checksum of $USERFS is    $CHECK"

CRC=`$crc $RAWUSER`
[ $? -ne 0 ] && eval "$Failmsg" && eval "$cleanup" && exit 1
echo "Checksum of $USER is $CRC"

if [ $CHECK != $CRC ]; then
	echo "Checksum miscompare." 
	eval "$Failmsg"; eval "$cleanup"; exit 1
else
	echo ""
	echo "Checksum matches; $USER PASSES!"
fi
 

if [ "$foption" ]; then
	echo ""
	echo "Modifying fstab to make usr read-only ..."
	test -d $MNTROOT || mkdir $MNTROOT
	echo "Mounting $ROOT on $MNTROOT ..."
	while :
	do
		$mount $ROOT $MNTROOT 
		if [ $? != 0 ]; then
			echo "Unable to mount $MNTROOT !"
			echo "Try again? (y/n) "
			eval "$get_yn" && continue || eval "$cleanup" && exit 1
		fi
		break;
	done
	[ ! -f $MNTROOT/etc/fstab ] && \
		echo "Unable to locate $MNTROOT/etc/fstab" && \
		eval "$cleanup" && exit 1
	rm -f $MNTROOT/etc/fstab- 
	mv $MNTROOT/etc/fstab $MNTROOT/etc/fstab-
	sed -e "/${DISK}g/s/rw/ro/" $MNTROOT/etc/fstab- > $MNTROOT/etc/fstab
	echo "fstab is now:"
	cat $MNTROOT/etc/fstab
	$umount $MNTROOT
	rmdir $MNTROOT > /dev/null 2>&1
fi


echo ""
echo "***** Exiting $CMDNAME Version %I%  `date` *****"
echo ""
eval "$cleanup"; exit 0
