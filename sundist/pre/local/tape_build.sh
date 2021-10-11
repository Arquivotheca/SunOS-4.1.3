#! /bin/sh
#
# %Z%%M% %I% %E% SMI
#
# tape_build
#   Generating pre-installed release tape from dd images
#       This script assumes:
#       1)  2 tapes will be created.  One for the disk-building scripts
#               and one for the disk images.
#       2)  The place where the above files are kept is a dir called /golden.
#
#
# Constants
	GOLDEN=/golden
	COPYRIGHT=/tmp/Copyright
	thisdir=$PWD
	cleanup='echo "Exiting";'

# Variables
	arch=
	release=
	disk=
	tape=rmt8

# Program

if [ $# -ne 0 ]; then
	if [ $1 = "-t" ]; then
		tape=$2
	fi
fi

#	Note we use r??8 for the higher tape densities
case $tape in
	rst8)	;;
	rmt8)	;;
	rst0)	;;
	*)
		echo "$tape not valid tapedrive name."; exit 1;;
esac

# Determine which architecture is to be loaded to tape
while true
do
	echo -n "Architecture to load to tape (sun4c, sun3x): "
	read arch
	case "$arch" in
		sun4m|sun4c|sun3x)
			break;;
		*)	echo "$arch is not a valid entry";;
	esac
done

# Determine which release is to be loaded to tape
echo -n "Release name (e.g. 4.1, pie): "
read release
[ -d $GOLDEN/$release/$arch ] || {
	echo "$GOLDEN/$release/$arch does not exist"
	exit
}

# Determine which disk drive model is to be loaded to tape (100/200mb)
while true
do
	echo " [ 1 ] 100mb Quantum ProDrive 105S"
	echo " [ 2 ] 200mb SUN0207"
	echo " [ 3 ] 424mb SUN0424"
	echo -n "Select Drive Model enter [ 1, 2, or 3 ]: "
	read ans
	case "$ans" in
		1)	size=100;disk=Quantum_ProDrive_105S;break;;
		2)	size=200;disk=SUN0207;break;;
		3)	size=424;disk=SUN0424;break;;
		*)	echo;;
	esac
done

[ -d $GOLDEN/$release/$arch/$disk ] || {
	echo "$GOLDEN/$release/$arch/$disk is not a directory"
	eval "$cleanup" 
	exit 1
	}

#	Check for existence of copyright files
if [ ! -s Copyright_${arch}_${size} ]; then
	echo "Copyright file not present."
	eval "$cleanup" 
	exit 1
fi

#	Create disk image tape
echo "Insert tape #1 and hit <return>"
read ret

# Remove the copyright file from /tmp if it exists 
cd $thisdir
[ -f $COPYRIGHT ] && rm -f $COPYRIGHT
cp Copyright_${arch}_${size} $COPYRIGHT

# Use dd to load the copyright to tape (1st file)
echo "Loading Copyright header Copyright_${arch}_${size}"
mt -f /dev/$tape rewind
dd if=$COPYRIGHT of=/dev/n$tape conv=sync 
[ $? -ne 0 ] && echo "Unable to write to tape." && eval "$cleanup" && exit 1

# Use tar to append the following to tape
echo "Loading the images"
cd $GOLDEN
tar cvpf /dev/n$tape $release/$arch/$disk 

# Use dd again this time to append the copyright to the end of the tape
echo "Loading Copyright trailer Copyright_${arch}_${size}"
dd if=$COPYRIGHT of=/dev/n$tape conv=sync 
mt -f /dev/$tape rewind

# Ask if the person wants to build a tools tape
while true
do
	echo -n "Build tools tapes [y/n]: "
	read ans
	case "$ans" in
		y)	break;;
		n)	
			echo "Pre-install tapes created on `date`";
			eval "$cleanup" && exit 0;;
		*)	;;
	esac
done


#	Create tools tape
echo "Insert tape #2 and hit <return>"
read ret

# Remove the copyright file from /tmp if it exists 
cd $thisdir
[ -f $COPYRIGHT ] && rm -f $COPYRIGHT
cp Copyright_scripts $COPYRIGHT

# Use dd to load the copyright to tape (1st file)
echo "Loading Copyright header Copyright_scripts"
mt -f /dev/$tape rewind
dd if=$COPYRIGHT of=/dev/n$tape conv=sync 
[ $? -ne 0 ] && echo "Unable to write to tape." && eval "$cleanup" && exit 1

# Use tar to append the following to tape
echo "Loading the tools"
cd $GOLDEN
tar cvpf /dev/n$tape tools

# Use dd again this time to append the copyright to the end of the tape
echo "Loading Copyright trailer Copyright_scripts"
dd if=$COPYRIGHT of=/dev/n$tape conv=sync 
mt -f /dev/$tape rewind

echo "Pre-install tapes created on `date`"
eval "$cleanup" && exit 0
