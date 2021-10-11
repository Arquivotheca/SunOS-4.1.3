#
# @(#)extract.cdrom.sh 1.1 92/07/30 Copyright 1989 Sun Microsystems
# remember, this is sourced by .profile, nee Install.miniroot.sh
# input variable "disk" set to which disk gets loaded with miniroot
#  the following are prepended to this script via getmunixfs.sh
# input variable "miniskip" (prepended) in bytes, where miniroot
#	is in architecture boot partition
# input variable "minicount" (prepended) in bytes, sizeof miniroot
#
# loadfrom string from getchardev/ramdisk is found in first part of ramdisk
# it is the unix style device name
loadfrom=`dd if=/dev/rd0a bs=1b count=1 2>/dev/null | ( read aword junk; echo $aword )`
#
case $loadfrom in
sd*|sr*) # is cdrom or scsi disk masqarading as one
	cddev='/dev/rsr0'
	cdpartno=`expr $loadfrom : '[a-zA-Z]*[0-9]\([a-h]\)' `
	case $cdpartno in
	a) echo "ERROR: a is invalid cdrom partition for booting"; break 2;;
	b) cdpartno=1;;
	c) cdpartno=2;;
	d) cdpartno=3;;
	e) cdpartno=4;;
	f) cdpartno=5;;
	g) cdpartno=6;;
	h) cdpartno=7;;
	*) echo "ERROR: bad parse of cdpartno: $cdparno"; break 2;;
	esac
	echo "using cdrom partition number $cdpartno"
	fastread $cddev $cdpartno $miniskip $minicount > /dev/r${disk}b
	if [ $? -ne 0 ]; then
		echo "ERROR while loading miniroot disk: /dev/r${disk}b"
		break 2
	fi
	;;
*)	# unknown
	echo ""
	echo "ERROR: unknown loadfrom: \"$loadfrom\" "
	echo ""
	break 2 ;;
esac

