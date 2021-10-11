#!/bin/sh
# %Z%%M% %I% %E% Copyright 1989 Sun Microsystems, Inc.
#
# Make an munix to boot off of a given device, with the root and swap and file
# number to load munixfs already set. 
# CAUTION: Be *extremely* careful not to echo anything on stdout, or it will
# show up in the tape file!
# This is *only* for tapes; diskette and cdrom make their MUNIX in their
# private makefiles: diskette.mk and cdrom.mk.
#
# args:
# 0	- myname
# 1	- MUNIX kernel to start with
# 2	- tape device
#

if [ $# -ne 2 ]; then
	echo "usage: $0 file tapedev" >> /dev/tty
	exit 1
fi

# temp file name
munix="munix.$$"
cp $1 $munix

#
# now everybody's MUNIX will come up with no questions
# (Unless you boot -a)
#
rootfs="4.2"
rootdev="rd0a"
swapfs="spec"
swapdev="ns0b"
#
# XXX - munixfs is "always" the third file
# NOTE: sun4c munixfs is preloaded, this is impossible for other arch's,
# would be nice to figure this out from the XDRTOC.
#
loadfile="3"

echo "rootfs: \"${rootfs}\", rootdev: \"${rootdev}\"" >> /dev/tty
echo "swapfile: \"${swapfs}\", swapdevice: \"${swapdev}\"" >> /dev/tty
echo "loadramdiskfile: ${loadfile}" >> /dev/tty

( ( echo "rootfs?W '${rootfs}'" ; \
  echo "rootfs+10?W '${rootdev}'" ; \
  echo "swapfile?W '${swapfs}'" ; \
  echo "swapfile+10?W '${swapdev}'" ; \
  echo "loadramdiskfile?W ${loadfile}" ; \
  echo '$q'; ) | adb -w $munix ) >/dev/null 2>&1

#
# invoke makesaexec to put it on the tape
#
makesaexec $munix
rc=$?
rm $munix
exit $rc

