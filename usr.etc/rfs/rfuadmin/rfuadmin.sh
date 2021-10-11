#	@(#)rfuadmin.sh 1.1 92/07/30 SMI
#
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rfuadmin:rfuadmin.sh	1.12.2.1"

echo "#Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	\"@(#)/usr/bin/rfuadmin.sl 1.2 3.0 10/07/85 50322\"

# executed by rfudaemon on request from another system.
# this is a filter that allows other systems to execute
# selected commands. This example is used by the fumount
# command when a remote system is going down.
# System administrators may expand the case list if it is
# desired to add to the remote command list.


if [ -d /usr/adm/log ]
then
	LOG=/usr/adm/log/rfuadmin.log
else
	LOG=/usr/adm/rfuadmin.log
fi
echo \`date\` \$* >>\$LOG
case \$1 in
'fuwarn' | 'disconnect' | 'fumount')
	TYPE=\$1
	RESRC=\$2
	GRACE=\$3
	;;
'error')
	echo \$* >>\$LOG
	echo \$* >/dev/console 2>&1
	exit 1
	;;
*)
echo \"unexpected command \\\"\$*\\\"\">>\$LOG
	echo \"unexpected command for \$0 \\\"\$*\\\"\">/dev/console 2>&1
	exit 1
	;;
esac

		# RESRC is of the form domain.resource.

R=\`echo \$RESRC | sed -e 's/\\..*//'\`	# domain of the resource
D=\`/usr/bin/dname\`				# this domain
RESRC2=\`echo \$RESRC | sed -e 's/.*\\.//'\`	# resource name only

M=\`/etc/mount  |
	while read dev dummy1 fs dummy2 type options
	do
		if [ \"\${dev}\" = \"\${RESRC}\" ]
		then
				# if the full name is in the mount table,
				# it is unique.
			echo \$fs \$dummy1 \$dev \$options
			exit 0
		else
			# otherwise,
			# if the domain of this machine is the same
			# as the that of the resource, it may be
			# mounted without the domain name specified.
			# Must be careful here, cuz if the resource
			# is 'XXX', we may find 'other_domain.XXX'
			# as well as 'XXX' in the mount table.

			if [ \"\$R\" = \"\$D\" ]
			then
					# the domain is the same
				if [ \"\${RESRC2}\" = \"\${dev}\" ]
				then
					echo \$fs \$dummy1 \$dev \$options
					exit 0
				fi
			fi
		fi
	done\`
if [ \"\$M\" = \"\" ]
then
	exit 0		# it's not mounted
fi
M=\`echo \$M |  sed -e 's/(//' -e 's/)//' -e 's/bg,//' -e 's/,bg//'\`
set \$M
			# \$1 is mountpoint
			# \$3 is 'domain.resource' or 'resource'
			# \$4 is options
case \${TYPE} in

#		The fumount(1M) warning from a host
'fuwarn')
	if [ \"\$GRACE\" != \"0\" ]
	then
	echo \"\$1\" will be disconnected from the system in \$GRACE seconds.>>\$LOG
	/bin/wall <<!
'\$1' will be disconnected from the system in \$GRACE seconds.
!
	fi
	exit 0
	;;

'disconnect' | 'fumount')
	if [ \"\$TYPE\" = \"fumount\" ]
	then
		echo \"$1\" is being disconnected from the system NOW!>>\$LOG
		/bin/wall <<!
'\$1' is being disconnected from the system NOW!
!
	else
		echo \"\$1\" has been disconnected from the system.>>\$LOG
		/bin/wall <<!
'\$1' has been disconnected from the system.
!
	fi
	/etc/fuser -k \$3 >>\$LOG 2>&1
	echo umount -d \$3 >>\$LOG
	/etc/umount -d \$3
		# for automatic remount, ...
	sleep 10
	mount -d -o bg,\$4 \$3 \$1
	exit 0
	;;

esac

" >rfuadmin
