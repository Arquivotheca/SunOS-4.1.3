#! /bin/sh
#
#      %Z%  %M%  %I%  %E%  SMI
#
#	makesaexec file [ seek ]
#
#	write a massaged standalone image to standard output in
#	form for disk or tape
#
#	seek is used for disks
#
tfile=hole.tmp$$
minblksize=512
if [ $# -lt 1 -o $# -gt 2 ]
then
	echo "usage: $0 file [ seek ]" >> /dev/tty
	exit 1
elif [ $#  -eq 2 ]
then
	seek="seek=$2"
	file=$1
else
	seek=""
	file=$1
fi

tapefile $file
case $? in
	16 | 32 | 48 )
		pagesize=8k
		skip=1
		ibs=32 ;;
	0 | 1 )
		skip=1
		pagesize=2k
		ibs=32 ;;
	2 )
		skip=1
		pagesize=2k
		ibs=$pagesize ;;
	18 | 34 | 50 )
		skip=0
		pagesize=8k
		ibs=32 ;;
	17 | 33 | 49 )
		echo "Warning: NMAGIC files cannot be executed on tape" >> \
			/dev/tty
		exit -1 ;;
	* )
		echo "$file not an executable file" >> /dev/tty
		exit -1 ;;
esac
rm -f $tfile
dd if=$file of=$tfile ibs=$ibs obs=$pagesize skip=$skip
if [ $? -ne 0 ] ; then exit $? ; fi
dd if=$file bs=$minblksize count=1 conv=sync $seek
if [ $? -ne 0 ] ; then exit $? ; fi
dd if=$tfile bs=$pagesize conv=sync
if [ $? -ne 0 ] ; then exit $? ; fi
rm -f $tfile
exit 0
