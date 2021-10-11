#!/bin/sh
# invoked as memtest_unload id type bmajor cmajor
DEBUG=:
${DEBUG} echo memtest_uload $*
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

rm -f $special
rm -f $ID

exit 0
