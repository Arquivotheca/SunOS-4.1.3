#!/bin/sh
#%Z%%M% %I% %E% SMI

# Variables:

logfile=/usr/src/buildlogs/LOG.`date +%m.%d.%y`
host=$1

# Use the same kernels as defined in /usr/src/Makefile in the macro "KERNELS"
kernels="GENERIC GENERIC_SMALL MINIROOT MUNIX LINT"

[ -z "$host" ] && host=`hostname`


# Verify correct host to do build on

# Verify logging directory exist

[ -d `dirname $logfile` ] || mkdir `dirname $logfile`

# Set umask to 664 (rw-rw-r--)

umask 2

echo "Remove $kernels dirs in /usr/src/sys/`arch -k`"

cd /usr/src/sys/`arch -k`
for i in $kernels
do
	[ -d $i ] && rm -rf $i
done

echo "Remove $kernel config files in  /usr/src/sys/`arch -k`/conf"
cd /usr/src/sys/`arch -k`/conf
for i in $kernels
do
	[ -f $i ] && rm -f $i
done

echo "Build $kernels"
cd /usr/src/sys
/bin/time make   > $logfile 2>&1
buildstatus=$?

echo "
	OUTPUT - ${host}:${logfile} " >> $logfile

# Send the logfile via mail

if [ "$buildstatus" = 0 ]
then
	egrep -in '\*\*\*|warning|error:' $logfile | \
		mail -s "${host}: Kernel build " val-police@dgdo
else
	egrep -in '\*\*\*|warning|error:' $logfile | \
		mail -s "${host}: Kernel build FAILED" val-police@dgdo
fi
