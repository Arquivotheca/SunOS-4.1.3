#!/bin/sh
#
#	Parallel install into /proto
#
#	Note: must be run with root permission.
#
#	cd /usr/src ; makelibs -k &
#
#	Make stdout is collected into make.out
#	Make stderr is collected into make.err
#


F0=lang
F1="include usr.lib lib etc bin 5bin usr.etc ucb files usr.bin"
F2="ucbinclude ucblib sys old pub man"
F3="xpginclude xpglib xpgbin adm sccs games diagnostics demo"

RAW=`egrep /proto /etc/fstab | sed -e 's, .*,,' | sed -e 's,/dev/,/dev/r,'`
if [ ! -n "$RAW" ]; then
	echo "no device in fstab for /proto?"
	exit 1
fi

echo using $RAW

cd /usr/src

date > pmake.status

umount /proto
newfs $RAW > make.out 2>&1
mount /proto
chmod g+s /proto
chgrp staff /proto

make DESTDIR=/proto install_dirs >> make.out 2>&1
make -k links >> make.out 2>&1

case `mach` in
	mc68010|mc68020)	CPU=m68k;;
	sparc)			CPU=sparc;;
esac

DESTDIR=/proto
ARCH=`arch`
export CPU ARCH DESTDIR


for i in 0 1 2 3
do
	(
	date
	TARGETS=`eval echo \\$F$i`
	for j in $TARGETS; do cd $j; make -k DESTDIR=/proto install; cd ..; done
	date
	echo "#$i done: $TARGETS" >> pmake.status
	) > make.out.$i 2>&1 &
done

wait
(cd sundist; make -k install munixfs miniroot) > make.out.4 2>&1
cat make.out.? | fold -132 >> make.out
rm make.out.?
date >> pmake.status
