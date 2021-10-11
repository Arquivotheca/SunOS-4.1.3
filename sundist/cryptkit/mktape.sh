#! /bin/sh
# %Z%%M% %I% %E% SMI;
#
#	Ordering (by tape file):
#	1  - Copyright
#	2  - install_unbundled
#	3  - Sun3 / and /usr
#	4  - Sun3 Networking
#	5  - Sun3 Debugging
#	6  - Sun3 System V
#	7  - Sun3 Manual
#	8  - Sun4 / and /usr
#	9  - Sun4 Networking,
#	10 - Sun4 Debugging
#	11 - Sun4 System V
#	12 - Sun4 Manual
#	13 - Copyright

CRYPTKIT=/usr/src/CRYPTKIT
DIR=/miniroot
PWD=`pwd`
USREXCLUDE=$PWD/exclude.usr
DEBUGEXCLUDE=$PWD/exclude.debug 

prepfs()
{
	RAWDEV=`grep $DIR /etc/fstab |awk '{print $1}' $DEV |sed 's/dev\//&r/'`
	umount $DIR; newfs $RAWDEV; mount $DIR
	(cd $DIR; rm -rf lost+found; tar xfBpv $1)
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
#   Size files for floppy volumes
#
floppy_volume()
{
    FILES=`find * -type f -print`
    FF=""
    for f in $FILES
    do
      echo $f | grep "+install" > /dev/null
      if [ $? = 1 ]; then
        echo $f | grep "bin" > /dev/null
        if [ $? = 1 ]; then
          echo $f | grep "ucb" > /dev/null
          if [ $? = 1 ]; then
            echo $f | grep "lib/libc.so" > /dev/null
            if [ $? = 1 ]; then
              FF="$FF $f"
            fi
          fi
        fi
      fi
    done
    sz=0
    vol=1
    rm /tmp/floppyvol.*
    echo "+install" > /tmp/floppyvol.${vol}
    ts=`du -s +install|awk '{print $1}'`
    sz=`expr ${sz} + ${ts}`
    echo "bin" >> /tmp/floppyvol.${vol}
    ts=`du -s bin|awk '{print $1}'`
    sz=`expr ${sz} + ${ts}`
    echo "ucb" >> /tmp/floppyvol.${vol}
    ts=`du -s ucb|awk '{print $1}'`
    sz=`expr ${sz} + ${ts}`
    echo "lib/libc.so*" >> /tmp/floppyvol.${vol}
    ts=`du -s lib/libc.so*|awk '{print $1}'`
    sz=`expr ${sz} + ${ts}`
    for f in ${FF}
    do
      ts=`du -s ${f}|awk '{print $1}'`
      szn=`expr ${sz} + ${ts}`
      if [ ${szn} -gt 2250 ]; then
        vol=`expr ${vol} + 1`
        sz=0
      fi
      echo $f >> /tmp/floppyvol.${vol}
      sz=`expr ${sz} + ${ts}`
    done
}

#
#	START INSTALLATION HERE
#
#	Check for valid media device
#
if [ $# -lt 1 ]; then
	echo -n 'Enter media type: (st* | mt* | fd*)  '
	read MEDIA
else
	MEDIA=$1
fi
case "${MEDIA}" in
st? )
	BS=126
	MEDIATYPE=tape
	break;;
mt? )
	BS=20
	MEDIATYPE=tape
	break;;
fd? )
	BS=18
	MEDIATYPE=floppy
	break;;
* )
	echo "Device ${MEDIA} not supported, Exiting"
	exit 1;;
esac

#
# Make tapes or floppies
#
case $MEDIATYPE in
tape)
	SUN3=`find_cryptkit sun3`
	SUN4=`find_cryptkit sun4`
	#
	#	Start cutting the tape
	#
	mt -f /dev/r${MEDIA} rew
	dd if=Copyright of=/dev/nr${MEDIA} conv=sync
	tar cvfp /dev/nr${MEDIA} install_unbundled 4.1.1_US_Encryption
	#
	#	Package each architecture
	#
	for i in $SUN3 $SUN4; do
		prepfs /net/$i/$CRYPTKIT
		(cd $DIR/usr
		rename_libs
		tar cvfpbBXX /dev/nr${MEDIA} $BS $USREXCLUDE $DEBUGEXCLUDE \
		    bin etc include lib ucb
		tar cvfpbB /dev/nr${MEDIA} $BS `cat $USREXCLUDE`
		tar cvfpbB /dev/nr${MEDIA} $BS `cat $DEBUGEXCLUDE`
		tar cvfpbB /dev/nr${MEDIA} $BS 5lib
		tar cvfpbB /dev/nr${MEDIA} $BS share)
	done
	#
	#	Finish up
	#
	dd if=Copyright of=/dev/nr${MEDIA} conv=sync
	mt -f /dev/r${MEDIA} offline
	echo "US Encryption tape completed"
	break;;

floppy)
	echo "Making US Encryption floppies"
	echo -n "Enter floppy architecture (sun3|sun4): "
	read TYPE
	case $TYPE in
	sun3)	
		SUN3=`find_cryptkit sun3`
		prepfs /net/$SUN3/$CRYPTKIT;;
	sun4)
		SUN4=`find_cryptkit sun4`
		prepfs /net/$SUN4/$CRYPTKIT;;
	*)	
		echo "Unknown architecture \"$TYPE\""; exit 1;;
	esac
	mkdir $DIR/usr/+install
	cp install_unbundled 4.1.1_US_Encryption $DIR/usr/+install
	cp Copyright $DIR/usr/+install/copyright.d
	(cd $DIR/usr
	rename_libs
	if [ -f /tmp/floppyvol.1 ]; then
		echo "Previously sized floppy data exists"
		echo -n "Enter y to resize or n to keep old data:  "
		read ans
		case $ans in
		y)
		  floppy_volume;;
        *)
		  echo "Floppy volume resizing not done.";;
        esac
    else
		floppy_volume
    fi
	echo "Do you wish to manually change the contents of the"
	echo "floppy volumes?"
	echo -n "Enter y to manually change and n to continue:  "
	read ans
    case $ans in
	y)
	  vi /tmp/floppyvol.*;;
    *)
	  ;;
    esac
    disks=`ls -l /tmp/floppyvol.* | wc | awk '{print $1}'`
	d=1
	while (test $d -le $disks)
	do
	  echo
	  echo -n "Insert floppy number $d and enter return to continue:"
	  read ans
	  bar cvfpbZ /dev/r${MEDIA} $BS `cat /tmp/floppyvol.$d`
	  eject
	  d=`expr $d + 1`
    done)
	echo "${TYPE} US Encryption floppies completed"
	break;;

*)
	echo 'What kind of media are you making?!?'
	exit 1;;
esac
