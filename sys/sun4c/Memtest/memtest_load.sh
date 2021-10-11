#!/bin/sh
# invoked as memtest_load id type bmajor cmajor
DEBUG=:
${DEBUG} echo memtest_load $*
id=$1
type=$2
bmajor=$3
cmajor=$4

special=/dev/memtest
ID=memtestID

if [ $type != 12345607 ]
then
	echo "$0: unexpected type ($type, not 12345607)."
	exit 1
fi

echo $id > $ID

rm -f $special
mknod $special c $cmajor 0

${DEBUG} echo Driver $id is the memtest driver:
${DEBUG} modstat -id $id

exit 0
