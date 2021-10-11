#! /bin/sh
# %Z%%M% %I% %E% SMI;
#

CRYPTKIT=/usr/src/CRYPTKIT
DIR=/miniroot
PWD=`pwd`
PRODUCT=US_Encryption_Kit
ARCHES="sun4"

#
# 	Prepare the raw partition
#
prepfs()
{
	RAWDEV=`grep $DIR /etc/fstab |awk '{print $1}' $DEV |sed 's/dev\//&r/'`
	umount $DIR; newfs $RAWDEV || exit 1; mount $DIR
	(cd $DIR; rm -rf lost+found)
}

#
#	Install scripts, copyright, and cdm stuff
#
doinstall()
{
	install -d $DIR/bin
	install -d $DIR/$PRODUCT/_install
	install -d $DIR/$PRODUCT/_text
	install cdrom/cdm $DIR
	install cdrom/cdmanager $DIR
	install cdrom/cdruninstall $DIR/bin
	install cdrom/extract_unbundled $DIR/bin
	install cdrom/Session.icon $DIR/bin
	install cdrom/_info $DIR/$PRODUCT
	install cdrom/4.1.3_US_Encryption $DIR/$PRODUCT/_install
	install copyright.r $DIR/$PRODUCT/_install
	install install_unbundled $DIR/$PRODUCT/_install
}

#
#	Rename libc's to include .us extension
#
rename_libs()
{
	for dir in lib 5lib; do
		if [ ! -d $dir -o ! -w $dir ]; then
			echo "Directory: ${dir} must be writable"
			echo "Exiting, modify permissions and restart"
			exit 1
		fi
		(cd $dir; for lib in libc*; do
			b=`basename ${lib} .us`
			if [ "${b}" = "${lib}" ]; then
				mv ${lib} ${lib}.us
			fi
		done)
	done
}

#
#	Find and check CRYPTKIT files
#
find_cryptkit()
{
	echo -n "$1 build machine? " > /dev/tty
	read WHERE
	if [ ! -f /net/$WHERE/$CRYPTKIT ]; then
		echo "/net/$WHERE/$CRYPTKIT not found; exiting" > /dev/tty
		exit 1
	fi
	echo $WHERE
}


# 
# 	Start of main program
#
prepfs 
doinstall

#
# 	Install CRYPTKIT files
#
for a in $ARCHES; do
	machine=`find_cryptkit $a`
	cd $DIR/$PRODUCT
	tar xvf /net/$machine/$CRYPTKIT 
	mv usr $a; cd $a
	chmod 777 lib 5lib
	rename_libs
done

echo "Done building $DIR."

