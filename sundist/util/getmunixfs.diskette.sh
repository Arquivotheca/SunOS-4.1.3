#! /bin/sh
#
#       %W% %E% Copyright 1989 Sun Microsystems, Inc
#
# Shell script to build a mini-root (acutally a munix ramdisk)
# file system for floppy disk in preparation for building a set
# of distribution floppies.
# The file system created here is image copied onto a single diskette,
# then image copied into the ramdisk as the "first"
# step in a cold boot of 4.1b-4.2 systems.
# We must be very carefull about what goes on the diskette, a 3.5"
# diskette only holds 80 * 2 * 18 = 2880 blocks = 1440 Kbytes.
#
DISTROOT=/proto
SUNDISTPATH=`dirname $0`
#
if [ `pwd` = '/' ]
then
	echo You almost destroyed the root
	exit
fi
date > .MUNIXFS
rm -rf bin; mkdir bin
rm -rf sbin; mkdir sbin
rm -rf etc; mkdir etc
rm -rf stand; mkdir stand
rm -rf a; mkdir a
rm -rf tmp; mkdir tmp; chmod 777 tmp
rm -rf usr 
mkdir usr usr/tmp usr/ucb usr/bin usr/lib usr/etc usr/kvm
ln -s usr/lib lib

# toss lost+found - save 16 blocks
rm -rf lost+found

# Copy dynamic loading and shared library environment.  
# Note that any shared library on which the components
# of the mini-root depends must be included here.
cp $DISTROOT/lib/ld.so lib
cp $DISTROOT/lib/libc.so.*.* lib
cp $DISTROOT/lib/libc.sa.*.* lib
ranlib lib/libc.sa.*.*

# Copy individual components.
# Not very much space, so don't copy anything not needed
cp $DISTROOT/etc/mtab etc
cp /dev/null etc/rc.boot
cp $DISTROOT/usr/etc/dkinfo etc; strip etc/dkinfo
cp $DISTROOT/usr/etc/init sbin; strip sbin/init
cp $DISTROOT/usr/etc/mknod usr/etc; strip usr/etc/mknod
cp $DISTROOT/usr/etc/format usr/etc; strip usr/etc/format
cp $DISTROOT/etc/format.dat etc
cp $DISTROOT/etc/reboot etc; strip etc/reboot
if [ $MINIARCH = sun4c ]; then
	cp $DISTROOT/usr/kvm/unixname2bootname usr/kvm
fi
#
# link these instead of copying, save 9 blocks!
cp $DISTROOT/usr/bin/true bin
cp $DISTROOT/usr/bin/false bin
ln bin/false bin/sun2
ln bin/false bin/sun386
ln bin/false bin/i386
if [ $MINIARCH = sun4c ]; then
	ln bin/false bin/sun3
	ln bin/false bin/sun3x
	ln bin/false bin/m68k
	ln bin/false bin/sun4
	ln bin/true bin/sun4c
	ln bin/true bin/sparc
fi
#
if [ $MINIARCH = sun3x ]; then
	ln bin/false bin/sun3
	ln bin/true bin/sun3x
	ln bin/true bin/m68k
	ln bin/false bin/sun4
	ln bin/false bin/sun4c
	ln bin/false bin/sparc
fi
#
cp $DISTROOT/usr/bin/ls bin; strip bin/ls
cp $DISTROOT/usr/bin/sh bin; strip bin/sh
cp $DISTROOT/usr/bin/mt bin; strip bin/mt
# use cat, rm instead #cp $DISTROOT/usr/bin/mv bin; strip bin/mv
cp $DISTROOT/usr/bin/sync bin; strip bin/sync
cp $DISTROOT/usr/bin/cat bin; strip bin/cat
cp $DISTROOT/usr/bin/echo bin; strip bin/echo
cp $DISTROOT/usr/bin/rm bin; strip bin/rm
# too big, use cat instead.   cp $DISTROOT/usr/bin/cp bin; strip bin/cp
cp $DISTROOT/usr/bin/expr bin; strip bin/expr
# need eject with blasted floppies
cp $DISTROOT/usr/bin/eject bin; strip bin/eject
# not using bar or tar anymore!, no space, nothing mountable
cp $DISTROOT/usr/bin/dd bin; strip bin/dd
cp $DISTROOT/usr/bin/ed bin; strip bin/ed
# too big, use ed instead.  cp $DISTROOT/usr/bin/grep bin; strip bin/grep
cp $DISTROOT/usr/bin/stty bin; strip bin/stty
cp $DISTROOT/usr/ucb/uncompress usr/ucb; strip usr/ucb/uncompress
#
# the profile is special - for ease of use
#	It comes up and asks what it should do - which is usually just
#	roll on the miniroot.
# NOTE: new way of doing miniroot: it is a compressed image put on
#	two diskettes, see diskette.mk, fdrolloff.c
#
cp $DISTROOT/.profile .profile
ARCH=${MINIONFLOPPY}
case ${ARCH} in
#   *"sun3x"*) 	blocksize=18 ; device=/dev/rfd0g ;;
    *"sun3x"*) 	blocksize=18 ; device=/dev/rfd0c ;;
    *"sun4c"*) 	blocksize=18 ; device=/dev/rfd0c ;;
    *) 		unknown ARCH \"\${ARCH}\"; exit 1 ;;
esac
echo "blocksize=${blocksize}" >> .profile
echo "device=${device}" >> .profile
#
# for each arch, put onto the disk a file "disks.known", which has all the
# known possible disks.
# we used to append to .profile, but 4.1 shell used too much memory
# the format is: diskname<space>commentary
#
if [ X${ARCH} = Xsun4c ]
then
echo 'sd0 the first one in the system unit' >> disks.known
echo 'sd1 the second one in the system unit' >> disks.known
echo 'sd2 external disk unit' >> disks.known
echo 'sd3 external disk unit' >> disks.known
echo 'sd4 external disk unit' >> disks.known
echo 'sd6 external disk unit' >> disks.known
fi #end of sun4c
#
# to deal with generic Install.miniroot script, eject floppy first thing
#
echo "eject $device" >> .profile
#
# now that the parameters are setup, append the main part of the script
#
cat ${SUNDISTPATH}/Install.miniroot >> .profile
#
# NOTE: extract's "device" parameter is set by virtue of variable expansion
# during the reading of this inline document
cat > extract << EOF-EXTRACT
#!/bin/sh
# requires that "disk" be set to sd0, sd1, etc.
{
	echo -n "insert diskette \"C\", press return when ready" > /dev/tty;
	read answer 2>&1 > /dev/tty;
	fdrolloff "C" $device
	eject 2> /dev/tty;
	echo -n "insert diskette \"D\", press return when ready" > /dev/tty;
	read answer 2>&1 > /dev/tty;
	fdrolloff "D" $device
	eject 2> /dev/tty;
	echo -n "insert diskette \"E\", press return when ready" > /dev/tty;
	read answer 2>&1 > /dev/tty;
	fdrolloff "E" $device
	eject 2> /dev/tty;
} | uncompress -c > /dev/r\${disk}b 2> /dev/tty
EOF-EXTRACT
#
# now how about a README file that is a short summary of above script?
#
cat >> README << EOF-README
README for creating a miniroot
1. use "format" to format and/or label the disk as needed 
2. "eject" the munixfs diskette ("B") if not already ejected 
3. set the variable "disk" to sd0, sd1 or whatever is appropriate
	example# disk=sd0
4. run the extract script by sourceing it
	example# . /extract
5. follow its instructions
6. when the shell prompts (with "#"), enter:
	reboot sd(0,<UNIT#>,1) -sw
  where "<UNIT#>" is the unit number, ie 0 for sd0, 1 for sd1, etc.
  if that fails, do an L1-A or power cycle to reboot.
EOF-README
#
cp ${SUNDISTPATH}/fdrolloff etc; strip etc/fdrolloff
cp $DISTROOT/etc/hosts etc
cp $DISTROOT/etc/ttytab etc
cp $DISTROOT/etc/services etc

cat >etc/passwd <<EOF
root::0:10::/:/bin/sh
EOF
cat >etc/group <<EOF
wheel:*:0:
kmem:*:2:
operator:*:5:
staff:*:10:
EOF
rm -rf dev; mkdir dev
cp /proto/dev/MAKEDEV dev
chmod +x dev/MAKEDEV
cp /dev/null dev/MAKEDEV.local
cd dev
./MAKEDEV std st0 st1 st2 st3
./MAKEDEV sd0 sd1 sd2 sd3 sd4 sd6
# fd0 is made by "std" on machines that have floppies
cd ..
sync
