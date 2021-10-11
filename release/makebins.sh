#!/bin/sh
#
#	Parallel make of binaries
#
#	NOTE: The list of targets in F0 - F3 must include
#	all the targets given in the "all" dependency
#	list for /usr/src/Makefile except for lib, 5lib and usr.lib.
#	Those targets are handled by the makelibs script.
#
#	cd /usr/src ; makebins -k &
#
#	Make stdout is collected into make.out
#	Make stderr is collected into make.err
#

F0="usr.bin"
F1="include ucbinclude etc bin usr.etc sundist"
F2="sys old pub man files"
F3="lang ucb adm sccs games diagnostics demo"

case `mach` in
	mc68010|mc68020)	CPU=m68k;;
	sparc)			CPU=sparc;;
esac

ARCH=`arch`
export ARCH CPU

cd /usr/src

date > pmake.status

for i in 0 1 2 3
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
