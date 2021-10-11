#!/bin/sh -x
#
# new disk space sizing routine
# feed in a list of files (from the tarfiles),
# ignore any directories ??? or emit 1k for them?
#
#
set -x
if [ $# -ne 1 ]; then
	echo "$0: usage: $0 tarfile" > /dev/tty
	exit 1
fi
if [ ! -r $1 ]; then
	echo "$0: cannot read $1" > /dev/tty
	exit 1
fi


tar tf $1 2> /dev/tty | \
sed -e 's= symbolic link to .*==' -e 's= linked to .*==' 2> /dev/tty | \
while read fn
do
	if [ -d $fn ]; then
		# XXX emit some overhead quantity, like 2k?
		echo 2 $fn
		continue;
	fi
	if [ ! -r $fn ]; then
		continue;
	fi
	du -s $fn 2> /dev/tty
done | awk '{ sum += $1; } END { print sum "kb"; }' 2> /dev/tty

