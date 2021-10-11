#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	%Z%%M% %I% %E% SMI; from UCB 5.3 3/29/86
#

trap "rm -f /tmp/whatisx.$$ /tmp/whatis$$; exit 1" 1 2 13 15
MANDIR=${1-/usr/man}
rm -f /tmp/whatisx.$$ /tmp/whatis$$
if test ! -d $MANDIR ; then exit 0 ; fi
cd $MANDIR
top=`pwd`
for i in man1 man2 man3 man4 man5 man6 man7 man8 mann manl
do
	if [ -d $i ] ; then
		cd $i
	 	if test "`echo *.*`" != "*.*" ; then
			/usr/lib/getNAME *.*
		fi
		cd $top
	fi
done >/tmp/whatisx.$$
sed  </tmp/whatisx.$$ >/tmp/whatis$$ \
	-e 's/\\-/-/' \
	-e 's/\\\*-/-/' \
	-e 's/ VAX-11//' \
	-e 's/\\f[PRIB0123]//g' \
	-e 's/\\s[-+0-9]*//g' \
	-e '/ - /!d' \
	-e 's/.TH [^ ]* \([^ 	]*\).*	\(.*\) -/\2 (\1)	 -/' \
	-e 's/	 /	/g'
/usr/ucb/expand -24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100 \
	/tmp/whatis$$ | sort | /usr/ucb/unexpand -a > whatis
chmod 644 whatis >/dev/null 2>&1
rm -f /tmp/whatisx.$$ /tmp/whatis$$
exit 0
