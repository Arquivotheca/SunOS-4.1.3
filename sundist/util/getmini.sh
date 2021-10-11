#! /bin/sh
#
#       %Z%%M% %I% %E% SMI
#
# Shell script to build a mini-root file system
# in preparation for building a distribution tape.
# The file system created here is image copied onto
# tape, then image copied onto disk as the "first"
# step in a cold boot of 4.1b-4.2 systems.
#
DISTROOT=/proto
INSTALLBOOT=${DISTROOT}/usr/mdec/installboot
KERNEL=${DISTROOT}/usr/kvm/stand/vmunix_miniroot
SUNDISTPATH=`dirname $0`
#
if [ `pwd` = '/' ]
then
	echo You just '(almost)' destroyed the root
	exit 1
fi
if [ ! -d $DISTROOT ]; then
	echo No prototype root $DISTROOT
	exit 1
fi
date > .MINIROOT
cp ${KERNEL} vmunix
# 4.1_PSR_A needed lots of space, but sun4c getcons needs to find "_romp"
if [ ! X${MINIARCH} = Xsun4c -a ! X${MINIARCH} = Xsun4m ]; then
strip vmunix
fi
rm -rf bin; mkdir bin
rm -rf sbin; mkdir sbin
rm -rf etc; mkdir etc etc/install
rm -rf stand; mkdir stand
rm -rf a; mkdir a
rm -rf tmp; mkdir tmp; chmod 777 tmp
rm -rf usr 
mkdir usr usr/tmp usr/kvm usr/kvm/mdec usr/ucb usr/bin usr/lib usr/etc 
mkdir usr/share usr/share/lib usr/share/lib/keytables
ln -s usr/lib lib
ln -s kvm/mdec usr/mdec

# Copy dynamic loading and shared library environment.  
# Note that any shared library on which the components
# of the mini-root depends must be included here.
cp $DISTROOT/lib/ld.so lib
cp $DISTROOT/lib/libc.so.*.* lib
cp $DISTROOT/lib/libc.sa.*.* lib
ranlib lib/libc.sa.*.*
cp $DISTROOT/lib/libdl.so.1.0 lib

# Copy individual components.
cp $DISTROOT/etc/mtab etc
# stamp the release in here
echo "`arch`.`arch -k`.sunos.`cat $SUNDISTPATH/../../sys/conf.common/RELEASE`" > etc/install/release
cp $DISTROOT/etc/install/EXCLUDELIST etc/install 
cp $DISTROOT/etc/install/default_client_info etc/install 
cp $DISTROOT/etc/install/default_sys_info etc/install
cp $DISTROOT/etc/install/category.standalone etc/install
cp $DISTROOT/etc/install/category.repreinstalled etc/install
cp $DISTROOT/etc/install/label_script etc/install
cat > etc/rc.boot <<!
loadkeys -e
!
cp $DISTROOT/usr/etc/newfs etc; strip etc/newfs
cp $DISTROOT/usr/etc/dkinfo etc; strip etc/dkinfo
cp $DISTROOT/usr/etc/mkfs etc; strip etc/mkfs
cp $DISTROOT/usr/etc/restore etc; strip etc/restore
ln etc/restore etc/rrestore
cp $DISTROOT/usr/etc/init sbin; strip sbin/init
cp $DISTROOT/usr/etc/mount etc; strip etc/mount
cp $DISTROOT/usr/etc/mount_hsfs etc; strip etc/mount_hsfs
cp $DISTROOT/usr/etc/mkfile usr/etc; strip usr/etc/mkfile
cp $DISTROOT/usr/etc/route usr/etc; strip usr/etc/route
cp $DISTROOT/usr/etc/in.routed usr/etc; strip usr/etc/in.routed
cp $DISTROOT/usr/etc/mknod usr/etc; strip usr/etc/mknod
cp $DISTROOT/usr/etc/fsck etc; strip etc/fsck
cp $DISTROOT/usr/etc/umount etc; strip etc/umount
cp $DISTROOT/usr/etc/ifconfig etc; strip etc/ifconfig
cp $DISTROOT/usr/etc/fsirand etc; strip etc/fsirand
cp $DISTROOT/usr/etc/format usr/etc; strip usr/etc/format
cp $DISTROOT/etc/format.dat etc
cp $DISTROOT/etc/reboot etc; strip etc/reboot
cp $DISTROOT/usr/bin/fdformat usr/bin; strip usr/bin/fdformat
cp $DISTROOT/usr/bin/eject usr/bin; strip usr/bin/eject
cp $DISTROOT/usr/ucb/compress usr/ucb; strip usr/ucb/compress
ln usr/ucb/compress usr/ucb/uncompress
ln usr/ucb/compress usr/ucb/zcat
cp $DISTROOT/usr/bin/${MINIARCH} usr/kvm
cp $DISTROOT/usr/bin/sun2 bin
cp $DISTROOT/usr/bin/sun3 bin
cp $DISTROOT/usr/bin/sun3x bin
cp $DISTROOT/usr/bin/sun4 bin
cp $DISTROOT/usr/bin/sun4c bin
cp $DISTROOT/usr/bin/sun4m bin
cp $DISTROOT/usr/bin/sun386 bin
cp $DISTROOT/usr/bin/sparc bin
cp $DISTROOT/usr/bin/m68k bin
cp $DISTROOT/usr/bin/i386 bin
cp $DISTROOT/usr/bin/mach bin
cp $DISTROOT/usr/bin/arch bin
cp $DISTROOT/usr/bin/basename bin
cp $DISTROOT/usr/bin/hostid bin; strip bin/hostid
cp $DISTROOT/usr/bin/mt bin; strip bin/mt
cp $DISTROOT/usr/bin/ls bin; strip bin/ls
cp $DISTROOT/usr/bin/sh bin; strip bin/sh
cp $DISTROOT/usr/bin/mv bin; strip bin/mv
cp $DISTROOT/usr/bin/sync bin; strip bin/sync
cp $DISTROOT/usr/bin/cat bin; strip bin/cat
cp $DISTROOT/usr/bin/mkdir bin; strip bin/mkdir
cp $DISTROOT/usr/bin/stty bin; strip bin/stty; ln bin/stty bin/STTY
cp $DISTROOT/usr/bin/echo bin; strip bin/echo
cp $DISTROOT/usr/bin/rm bin; strip bin/rm
cp $DISTROOT/usr/bin/chgrp bin; strip bin/chgrp
cp $DISTROOT/usr/bin/cp bin; strip bin/cp
cp $DISTROOT/usr/bin/expr bin; strip bin/expr
cp $DISTROOT/usr/bin/awk bin; strip bin/awk
cp $DISTROOT/usr/bin/tar bin; strip bin/tar
cp $DISTROOT/usr/bin/dd bin; strip bin/dd
cp $DISTROOT/usr/bin/ed bin; strip bin/ed
cp $DISTROOT/usr/bin/hostname bin; strip bin/hostname
cp $DISTROOT/usr/bin/grep bin; strip bin/grep
cp $DISTROOT/usr/bin/sed bin; strip bin/sed
cp $DISTROOT/usr/bin/test bin; strip bin/test; ln bin/test 'bin/['
cp $DISTROOT/usr/bin/chmod bin; strip bin/chmod
cp $DISTROOT/usr/bin/date bin; strip bin/date
cp $DISTROOT/usr/bin/ln bin; strip bin/ln
cp $DISTROOT/usr/bin/loadkeys bin; strip bin/loadkeys
cp $DISTROOT/usr/ucb/rsh usr/ucb; strip usr/ucb/rsh
cp $DISTROOT/usr/ucb/rcp usr/ucb; strip usr/ucb/rcp
cp $DISTROOT/usr/ucb/whoami usr/ucb; strip usr/ucb/whoami
cp $DISTROOT/usr/kvm/mdec/* usr/kvm/mdec
cp $DISTROOT/usr/kvm/getcons usr/kvm/getcons
cp $DISTROOT/.profile .profile.tmp
sed 's/^\(PATH.*\)$/\1:\/usr\/etc\/install/' \
.profile.tmp > .profile; rm -f .profile.tmp
cat >> .profile <<!
#
# Probe for a "sun" terminal - really frame buffer and keyboard
#
getcons="/usr/kvm/getcons"
if [ -x \${getcons} ]; then
       \${getcons} | ( read wdin in wdout out; \
               if [ \$in -eq 0 -a \$out -eq 0 ]; then\
                       exit 0; \
               else \
                       exit 1; \
               fi; )
       if [ \$? -eq 0 ]; then
               TERM=sun
               export TERM
               si_gotfb="yes"
       else
               si_gotfb="no"
       fi
       export si_gotfb
fi
!

cp $DISTROOT/etc/hosts etc
cp $DISTROOT/etc/networks etc
cp $DISTROOT/etc/services etc
cp $DISTROOT/etc/protocols etc

(cd $DISTROOT/usr/etc; tar cf - install) | (cd usr/etc; tar xpBf -)
# remove usr/etc/install/configure so it will fit on floppy
rm -f usr/etc/install/configure
cp $DISTROOT/usr/ucb/tset usr/ucb
cp $DISTROOT/usr/share/lib/termcap usr/etc
cp $DISTROOT/usr/share/lib/keytables/layout_[0238][0-9a-f] usr/share/lib/keytables
(cd etc; ln -s /usr/etc/termcap)
#(cd $DISTROOT/usr/lib; tar vcf - tabset) | #(cd usr/lib; tar xpBf -)
mkdir usr/lib/fonts
#(cd $DISTROOT/usr/lib/fonts;tar vcf - fixedwidth*) | #(cd usr/lib/fonts;tar xpBf -)
#(cd $DISTROOT/usr/lib; tar vcf - defaults) | #(cd usr/lib/; tar xpBf -)
cp $DISTROOT/usr/etc/portmap etc
cp $DISTROOT/etc/ttytab etc
cp $DISTROOT/usr/bin/true bin
cp $DISTROOT/usr/bin/false bin
(cd $DISTROOT/usr/share/lib; tar cf - zoneinfo) | (cd usr/share/lib; tar xpBf -)


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
./MAKEDEV std
if [ ! X${MINIARCH} = Xsun4c -a ! X${MINIARCH} = Xsun4m ]; then
	./MAKEDEV xy0 xy1 xy2 xy3 xd0 xd1 xd2 xd3 xd4 xd5 xd6 xd7
	./MAKEDEV xd8 xd9 xd10 xd11 xd12 xd13 xd14 xd15	# all disks
fi
if [ X${MINIARCH} = Xsun4 -o X${MINIARCH} = Xsun4m ]; then
        ./MAKEDEV id000 id001 id002 id003 id004 id005 id006 id007
        ./MAKEDEV id010 id011 id012 id013 id014 id015 id016 id017
        ./MAKEDEV id020 id021 id022 id023 id024 id025 id026 id027
        ./MAKEDEV id030 id031 id032 id033 id034 id035 id036 id037
        ./MAKEDEV id040 id041 id042 id043 id044 id045 id046 id047 # all disks
fi
cd ..
# sun4c, cdrom ... boot miniroot directly - so do this for everybody
# added bootblks - "hv" says to add header for open boot proms
cp $DISTROOT/usr/stand/boot.${MINIARCH} boot.${MINIARCH}
BDEV=`basename $MINIDEV`
RDEV=/dev/r$BDEV
# syncs because fsync() doesn't work on indirect blocks!
sync;sync;sleep 8
# ahh, but OPENBOOT PROMs prefer to have a.out headers on files
if [ X${MINIARCH} = Xsun4c -o X${MINIARCH} = Xsun4m ]; then
${INSTALLBOOT} -hv boot.${MINIARCH} usr/mdec/rawboot ${RDEV}
else
${INSTALLBOOT} -v boot.${MINIARCH} usr/mdec/rawboot ${RDEV}
fi
sync;sync
