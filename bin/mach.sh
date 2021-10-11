#! /bin/sh
#     %Z%%M% %I% %E% SMI

for m in sparc mc68020 i386 mc68010
do
	if [ -f /bin/$m ] && /bin/$m ; then
		echo $m
		exit
	fi
done

echo unknown
