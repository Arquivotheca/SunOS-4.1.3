#!/bin/sh 
#	%Z%%M% %I% %E% SMI

#
# Given a preferred ordering of the object modules in the file $1, and a list
# of actual object modules in the directory $2, create a list of
# object modules for the "ld" command line. As time goes on, new modules 
# get created and old ones get deleted without the preferred list getting 
# updated. 
#
# The algorithm here is to delete unused object modules from the preferred
# ordering and then append the new modules to the resulting list.
#
# People should not determine the preferred list. Instead, it
# should be decided by a tool which orders the modules by dynamically most 
# used to least used. The idea is to get all of the "hot" object modules
# on the same page in order to reduce paging activity.
#

NEW=/tmp/sort.new.$$
OLD=/tmp/sort.old.$$
BOTH=/tmp/sort.both.$$
NEWONLY=/tmp/sort.newonly.$$
FIX=/tmp/sort.fix.$$
SAME=/tmp/sort.same.$$
SAMEORDERED=/tmp/sort.sameordered.$$

set -e

sed -e '/^#/d' $1 | sort > $OLD
cd $2
ls *.o > $NEW
cd ..
cat $OLD $NEW | sort > $BOTH
uniq -d $BOTH > $SAME
cat $SAME $NEW | sort | uniq -u > $NEWONLY
cat $SAME $OLD | sort | uniq -u | awk '{ print "/^" $1 "/d" }' > $FIX
if [ `wc -l < $FIX` = "0" ]; then
	sed -e '/^#/d' $1 > $SAMEORDERED
else
	sed -e '/^#/d' -f $FIX $1 > $SAMEORDERED
fi
cat $SAMEORDERED $NEWONLY | awk "{ print \"$2\" \"/\" \$1 }"

rm -f $OLD $NEW $BOTH $SAME $NEWONLY $FIX $SAMEORDERED
