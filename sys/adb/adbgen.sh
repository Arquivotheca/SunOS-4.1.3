#! /bin/sh
#
# %Z%%M% %I% %E% SMI
#
case $1 in
-d)
	DEBUG=:
	shift;;
-*)
	flag=$1
	shift;;
esac
ADBDIR=/usr/lib/adb
PATH=$PATH:$ADBDIR
for file in $*
do
	if [ `expr "XX$file" : ".*\.adb"` -eq 0 ]
	then
		echo File $file invalid.
		exit 1
	fi
	if [ $# -gt 1 ]
	then
		echo $file:
	fi
	file=`expr "XX$file" : "XX\(.*\)\.adb"`
	if adbgen1 $flag < $file.adb > $file.adb.c
	then
		if /bin/cc -w -D${ARCH:-`arch -k`} \
			-I../${ARCH:-`arch -k`} -I.. -I../sys \
			-o $file.run $file.adb.c $ADBDIR/adbsub.o
		then
			$file.run | adbgen3 | adbgen4 > $file
			$DEBUG rm -f $file.run $file.adb.C $file.adb.c $file.adb.o
		else
			$DEBUG rm -f $file.run $file.adb.C $file.adb.c $file.adb.o
			echo compile failed
			exit 1
		fi
	else
		$DEBUG rm -f $file.adb.C
		echo adbgen1 failed
		exit 1
	fi
done
