#! /bin/sh -
#
#	%Z%%M% %I% %E% SMI
#

#
# The first argument is ${INSTALL}, the second is ${DESTDIR}.
#
INSTALL=$1
VERS_FILE=$2
DESTDIR=$3
NONEX="-m 644"

#
# The black magic in VERSION_S5 adds one to the major version number in
# "version" and leaves the minor version number alone.  Isn't UNIX wonderful?
#
version_42=`cat $VERS_FILE`
version_s5=`expr \( $version_42 : '\(.*\)\..*' \) + 1`.`expr \( $version_42 : '.*\.\(.*\)' \)`

#
# Install S5 version in "/usr/lib" as "+<name>" with S5 version number.
#
$INSTALL libcs5.so $DESTDIR/usr/5lib/+libc.so.$version_s5
$INSTALL $NONEX libcs5.sa $DESTDIR/usr/5lib/+libc.sa.$version_s5

#
# Move S5 version currently in "/usr/5lib" to "-<name>".
#
if [ ! -n "$DESTDIR" -a -f $DESTDIR/usr/5lib/libc.so.$version_s5 ]; then \
	mv -f $DESTDIR/usr/5lib/libc.so.$version_s5 $DESTDIR/usr/5lib/-libc.so.$version_s5
fi
if [ ! -n "$DESTDIR" -a -f $DESTDIR/usr/5lib/libc.sa.$version_s5 ]; then \
	mv -f $DESTDIR/usr/5lib/libc.sa.$version_s5 $DESTDIR/usr/5lib/-libc.sa.$version_s5
fi

#
# Rename new S5 version from "+<name>" to "<name>", and "ranlib" the ".sa"
# file.
#
mv -f $DESTDIR/usr/5lib/+libc.so.$version_s5 $DESTDIR/usr/5lib/libc.so.$version_s5
mv -f $DESTDIR/usr/5lib/+libc.sa.$version_s5 $DESTDIR/usr/5lib/libc.sa.$version_s5
ranlib $DESTDIR/usr/5lib/libc.sa.$version_s5

#
# Install 42 version in "/usr/lib" as "+<name>" with 4.2 version number.
#
$INSTALL libc.so $DESTDIR/usr/lib/+libc.so.$version_42
$INSTALL $NONEX libc.sa $DESTDIR/usr/lib/+libc.sa.$version_42

#
# Move 4.2 version currently in "/usr/lib" to "-<name>".
#
if [ ! -n "$DESTDIR" -a -f $DESTDIR/usr/lib/libc.so.$version_42 ]; then \
	mv -f $DESTDIR/usr/lib/libc.so.$version_42 $DESTDIR/usr/lib/-libc.so.$version_42
fi
if [ ! -n "$DESTDIR" -a -f $DESTDIR/usr/lib/libc.sa.$version_42 ]; then \
	mv -f $DESTDIR/usr/lib/libc.sa.$version_42 $DESTDIR/usr/lib/-libc.sa.$version_42
fi

#
# Rename new 4.2 version from "+<name>" to "<name>", and "ranlib" the ".sa"
# file.
#
mv -f $DESTDIR/usr/lib/+libc.so.$version_42 $DESTDIR/usr/lib/libc.so.$version_42
mv -f  $DESTDIR/usr/lib/+libc.sa.$version_42 $DESTDIR/usr/lib/libc.sa.$version_42
ranlib $DESTDIR/usr/lib/libc.sa.$version_42

##
## Move old S5 version in "/usr/5lib" (it may be a real library, or it may be a
## symbolic link), and plant a new symbolic link from "/usr/lib" to the S5
## version in "/usr/5lib".
##
#if [ ! -n "$DESTDIR" -a -f $DESTDIR/usr/5lib/libc.so.$version_s5 ]; then \
#	mv $DESTDIR/usr/5lib/libc.so.$version_s5 $DESTDIR/usr/5lib/-libc.so.$version_s5
#fi
#if [ ! -n "$DESTDIR" -a -f $DESTDIR/usr/5lib/libc.sa.$version_s5 ]; then \
#	mv $DESTDIR/usr/5lib/libc.sa.$version_s5 $DESTDIR/usr/5lib/-libc.sa.$version_s5
#fi
#ln -s $DESTDIR/usr/lib/libc.so.$version_s5 $DESTDIR/usr/5lib/libc.so.$version_s5 
#ln -s $DESTDIR/usr/lib/libc.sa.$version_s5 $DESTDIR/usr/5lib/libc.sa.$version_s5 
