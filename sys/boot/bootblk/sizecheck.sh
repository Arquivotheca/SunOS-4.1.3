#!/bin/sh
# %Z%%M% %I% %E%
if [ $# -eq 0 ]; then
	bblock=a.out
else
	bblock=$1
fi
size=`size $bblock | tail -1 | awk ' { print $1 + $2; } '`
if [ $size -ge 7680 ]
then
	echo Boot block `expr $size - 7680` bytes too big!!
	exit 1
fi
echo Boot block is `expr 100 '*' $size '/' 7680`% full.
exit 0
