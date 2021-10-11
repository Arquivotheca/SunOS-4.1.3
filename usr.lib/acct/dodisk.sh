#! /bin/sh -
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"%Z%%M% %I% %E% SMI" /* from S5R3 1.7 */
# 'perform disk accounting'
_dir=/var/adm
_pickup=acct/nite
PATH=/usr/lib/acct:/bin:/usr/bin:/etc:
export PATH
while getopts o i
do
	case $i in
	o)	SLOW=1;;
	?)	echo "Usage: $0 [ -o ] [ filesystem ... ]"
		exit 1;;
	esac
done
shift `expr $OPTIND - 1`

cd ${_dir}

if [ "$SLOW" = "" ]
then
	if [ $# -lt 1 ]
	then
		args=`sed -n '/4.2/s/\([^ 	]*\)[ 	].*/\1/p' </etc/fstab`
	else
		args="$*"
	fi
	diskusg $args > dtmp
else
	if [ $# -lt 1 ]
	then
		args="/"
	else
		args="$*"
	fi
	for i in $args; do
		if [ ! -d $i ]
		then
			echo "$0: $i is not a directory -- ignored"
		else
			dir="$i $dir"
		fi
	done
	if [ "$dir" = "" ]
	then
		echo "$0: No data"
		> dtmp
	else
		find $dir -print | acctdusg > dtmp
	fi
fi

sort +0n +1 -o dtmp dtmp
acctdisk <dtmp >disktmp
chmod 644 disktmp
chown root disktmp
mv disktmp ${_pickup}/disktacct
