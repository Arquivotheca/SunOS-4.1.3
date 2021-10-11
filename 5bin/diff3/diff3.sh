#! /bin/sh
#
#	%Z%%M% %I% %E% SMI; from S5R2 1.2
#

PATH=/usr/5bin:/bin:/usr/bin
e=
case $1 in
-*)
	e=$1
	shift;;
esac
if test $# = 3 -a -f $1 -a -f $2 -a -f $3
then
	:
else
	echo usage: diff3 file1 file2 file3 1>&2
	exit
fi
trap "rm -f /tmp/d3[ab]$$" 0 1 2 13 15
diff $1 $3 >/tmp/d3a$$
diff $2 $3 >/tmp/d3b$$
/usr/5lib/diff3prog $e /tmp/d3[ab]$$ $1 $2 $3
