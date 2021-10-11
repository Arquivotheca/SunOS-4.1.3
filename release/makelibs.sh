#!/bin/sh
#
#	Parallel make of libraries: lib 5lib and usr.lib built simultaneously.
#
#	cd /usr/src ; makelibs -k &
#
#	Make stdout is collected into make.out
#	Make stderr is collected into make.err
#

F0="lib"
F1="ucblib"
F2="usr.lib"

case `mach` in
	mc68010|mc68020)	CPU=m68k;;
	sparc)			CPU=sparc;;
esac

ARCH=`arch`
export CPU ARCH

cd /usr/src

date > pmake.status

for i in 0 1 2
do
	(
	date
	TARGETS=`eval echo \\$F$i`
	for j in $TARGETS; do cd $j; make -X $*; cd ..; done
	date
	echo "#$i done: $TARGETS" >> pmake.status
	) > make.out.$i 2> make.err.$i&
done

wait
cat make.out.? | fold -132 > make.out
cat make.err.? | fold -132 > make.err
rm make.out.? make.err.?
date >> pmake.status
