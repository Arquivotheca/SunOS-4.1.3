#!/bin/csh -f
# 
#	%Z%%M% %I% %E%
#
# script run nightly to check for build errors
#
set path = (. /bin /usr/bin /usr/ucb /etc /usr/etc)
set outfile = /var/adm/build/nightly.output
set errfile = /var/adm/build/nightly.errors
set warnfile = /var/adm/build/nightly.warnings
if (-e /var/adm/build/nonightly) then
	echo "no build done on `date`"
	exit 0
endif
/var/adm/build/build >& $outfile
set bstatus = $status
if ($bstatus != 0) then
	echo "build failed on `date`; status = $bstatus"
else
	grep -i -n -w error $outfile > $errfile
	grep -i -n -w warning $outfile > $warnfile
	set e = `wc -l $errfile`
	set w = `wc -l $warnfile`
	echo "build finished on `date` with $e[1] errors, $w[1] warnings"
endif
