#! /bin/sh
#
#       %Z%%M% %I% %E% SMI
#
# Shell script to build a sun internal mini-root file system
# in preparation for building a distribution tape.
# The file system created here is image copied onto
# tape, then image copied onto disk as the "first"
# step in a cold boot of 4.1b-4.2 systems.
#
# XXX - should figure this out from the input parameters somehow
#
DISTROOT=/proto
MINIGET=/usr/src/sundist/bin/miniget
#
source $MINIGET
#
# Sun internal additions to standard stuff
cp /sys/dist/release/upgrscripts/upgrade_* .
chmod +x upgrade_*
cp /etc/hosts etc/hosts
cp $DISTROOT/bin/tee bin/tee
strip bin/tee
cp $DISTROOT/usr/etc/route usr/etc/route
strip usr/etc/route
cp $DISTROOT/etc/in.routed etc/in.routed
strip etc/in.routed
cd ..
sync
