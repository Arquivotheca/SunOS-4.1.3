#! /bin/sh -
#	%Z%%M% %I% %E% SMI

#
# The first argument is ${INSTALL}, the second is ${DESTDIR}.
#
INSTALL=$1
VERS_FILE=$4
DESTDIR=$5

#
# The black magic in VERSION_S5 adds one to the minor version number in
# "version" and leaves the minor version number alone.
#
# Stop auto incrementing the minor version number
# Janice C. Hsia - 7/5/88
#sccs edit $VERS_FILE
version_42=`cat $VERS_FILE`

#version_s5=`expr \( $version_42 : '\(.*\)\..*' \)`.`expr \( $version_42 : '.*\.\(.*\)' \) + 1`
version_s5=$version_42

#echo $version_s5 > $VERS_FILE
#sccs delget -y $VERS_FILE

 
$INSTALL libsuntool.so $DESTDIR/usr/lib/libsuntool.so.$version_s5
$INSTALL -m 644 libsuntool.sa $DESTDIR/usr/lib/libsuntool.sa.$version_s5
ranlib $DESTDIR/usr/lib/libsuntool.sa.$version_s5
