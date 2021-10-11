#! /bin/sh -

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"%Z%%M% %I% %E% SMI" /* from S5R3 acct:acct.sh 1.2 */

state=$1
set `who -r`
if [ $8 != "0" ]
then
	exit
fi
case $state in

'start')
	if [ $9 = "2" -o $9 = "3" ]
	then
		exit
	fi
	echo "Starting process accounting"
	/usr/lib/acct/startup
	;;
'stop')
	echo "Stopping process accounting"
	/usr/lib/acct/shutacct
	;;
esac
