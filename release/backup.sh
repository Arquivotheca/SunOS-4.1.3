#!/bin/sh
#%Z%%M% %I% %E% SMI

#!/bin/sh

# Description:
#	This script is used to backup a 4.1.x build system. 

# Note:
#	Update REMOTEHOST, DEV, and FS as needed


# Constants:
	FS="/tarfiles /proto /usr/src /sunupgrade_tmp / /usr"
	USAGE="`basename $0` backup_host"

# Test arguments


if [ $# -ne 1 ]
then
	echo "Error: $USAGE"
	exit
fi

case $1 in
	colander|eichler|valis)
		DEV=rst0;break;;
	archytas)
		DEV=rst1;break;;
	*)
		echo "Error: backup host not in list";
		exit;;
esac

REMOTEHOST=$1

echo "Rewinding Tape on $REMOTEHOST:/dev/$DEV"
rsh -n $REMOTEHOST "mt -f /dev/$DEV rew"

echo "Start Dump of `hostname` partitions:\"$FS\""
for i in $FS
do
	dump 0ubsdf 126 6000 54000 $REMOTEHOST:/dev/n$DEV $i
done

echo "Rewinding Tape on $REMOTEHOST:/dev/$DEV"
rsh -n $REMOTEHOST "mt -f /dev/$DEV offl"
echo "DUMP is DONE"
