#!/bin/sh
#
# %Z%%M% %I% %E% SMI
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#
#
#	tapeimage - force alignment of an executable to a certain
#	record and pagesize
#

if [ $# -ne 2 ]
then
	echo "usage: tapeimage file blocksize"
else
	file=$1
	tbs=$2
fi

minblksize=512

tapefile $file
case $? in
	16 | 32 )
		pagesize=8k
		skip=1
		ibs=32 ;;
	0 | 1 )
		pagesize=2k
		skip=1
		ibs=32 ;;
	2 )
		pagesize=2k
		skip=1
		ibs=$pagesize ;;
	18 | 34 )
		pagesize=8k
		skip=0
		ibs=32 ;;
	17 | 33 )
		echo "Warning: NMAGIC files cannot be executed from tape"
		exit -1 ;;
	*)
		echo $file "not an executable file";
		exit -1 ;;
esac


cp /dev/null hold.filea
cp /dev/null hold.fileb

dd if=$file of=hold.fileb ibs=$ibs obs=$pagesize skip=$skip
dd if=$file bs=$minblksize count=1 | tapeblock hold.filea $tbs
cat hold.filea hold.fileb
exit 0
