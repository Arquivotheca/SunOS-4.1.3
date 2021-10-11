#!/bin/sh
#
#       %Z%%M% %I% %E% SMI
#
# Check for file in the exclude lists which are not in /proto
#
cd /proto/usr
for i in /usr/src/sundist/exclude.lists/usr.* ; do
	ls `cat $i` > /dev/null 2>> /tmp/missing
done
cat /tmp/missing
rm /tmp/missing
