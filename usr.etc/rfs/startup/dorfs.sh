#! /bin/sh
#
# 	Start/stop RFS automatically 
#
#
#       %Z%  %M% %I% %E%
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#

HOME=/; export HOME
PATH=/bin:/usr/bin:/etc:/usr/etc:/usr/ucb

CMDNAME=$0
USAGE="usage: $0 <start [-v] | stop | init <domain> <netspec> [addr]>"

#
# Verify arguments
#
if [ $#  -lt 1 ]; then
	echo $USAGE
	exit 1
fi
OP=$1
case "$OP" in
"start" )
	if [ $# -lt 1 -o $# -gt 2 ]; then
        	echo "${CMDNAME}: incorrect number of arguments."
		echo $USAGE
        	exit 1
	else
		if [ -f /usr/nserve/rfmaster -a -f /usr/nserve/domain -a \
			-f /usr/nserve/netspec ]; then
			if [ $# -eq 2 -a "$2" = "-v" ]; then
				VFLAG="-v"
			else
				VFLAG=""
			fi
			NETSPEC=`cat /usr/nserve/netspec`
		fi
	fi
	break ;;
"stop" )
        if [ $# -ne 1 ]; then
                echo "${CMDNAME}: incorrect number of arguments."
                echo $USAGE
                exit 1    
        else
		if [ -f /usr/nserve/rfmaster -a -f /usr/nserve/domain -a \
                        -f /usr/nserve/netspec ]; then
                	NETSPEC=`cat /usr/nserve/netspec`
		fi
        fi
        break ;;
"init" )
	if [ $# = 3 ] ; then
		DOMAIN=$2
		NETSPEC=$3
		ADDR=5200
	elif [ $# = 4 ] ; then
		DOMAIN=$2
		NETSPEC=$3
		ADDR=$4
	else
        	echo "${CMDNAME}: incorrect number of arguments."
        	echo $USAGE 
        	exit 1
	fi 
	break ;;
* )
	echo "${CMDNAME}: incorrect option specified."
	echo $USAGE
	exit 1 ;;
esac

case "$OP" in
"init" )
        #
        # Initialize domain and netspec
        #
        rm -f /usr/nserve/domain /usr/nserve/netspec
        dname -D $DOMAIN 
        if [ $? != 0 ]; then
                echo
                echo "${CMDNAME}: can not start dname."
		echo "${CMDNAME}: RFS not started."
                exit 1
        fi
        dname -N $NETSPEC
        if [ $? != 0 ]; then 
                echo 
                echo "${CMDNAME}: can not start dname." 
                echo "${CMDNAME}: RFS not started." 
                exit 1 
        fi

        #
        # Initialize netspec
        #
        nlsadmin -i $NETSPEC
	if [ $? != 0 ]; then 
                echo 
                echo "${CMDNAME}: can not initialize $NETSPEC."
                echo "${CMDNAME}: RFS not started."
                exit 1 
        fi
	if [ $NETSPEC = "npack" ] ; then
        	nlsadmin -l \\x00000001${ADDR}0000 $NETSPEC
		if [ $? != 0 ]; then  
                	echo  
 ad              	echo "${CMDNAME}: can not execute nlsadmin." 
                	echo "${CMDNAME}: RFS not started." 
                	exit 1  
        	fi
        	nlsadmin -t \\x00000002${ADDR}0000 $NETSPEC
		if [ $? != 0 ]; then   
                	echo   
                	echo "${CMDNAME}: can not execute nlsadmin."  
                	echo "${CMDNAME}: RFS not started."  
                	exit 1   
        	fi
	fi 
	if [ $NETSPEC = "tcp" ] ; then
        	nlsadmin -l `hostrfs \`hostname\` $ADDR` $NETSPEC
		if [ $? != 0 ]; then  
                	echo  
                	echo "${CMDNAME}: can not execute nlsadmin." 
                	echo "${CMDNAME}: RFS not started." 
                	exit 1  
        	fi
        	nlsadmin -t `hostrfs \`hostname\` \`expr $ADDR + 1\`` $NETSPEC
		if [ $? != 0 ]; then   
                	echo   
                	echo "${CMDNAME}: can not execute nlsadmin."  
                	echo "${CMDNAME}: RFS not started."  
                	exit 1   
        	fi
	fi 
	if [ -f /var/net/nls/${NETSPEC}/dbf ]; then
        	grep rfsetup /var/net/nls/${NETSPEC}/dbf >/dev/null
        	if [ $? = 1 ]; then
                	nlsadmin -a 105 -c /usr/net/servers/rfs/rfsetup -y "RFS Server" $NETSPEC
			if [ $? != 0 ]; then  
                		echo  
                		echo "${CMDNAME}: can not execute nlsadmin."
                		echo "${CMDNAME}: RFS not started." 
                		exit 1   
        		fi
		fi
	else
		echo "${CMDNAME}: /usr/net/nls/npack/dbf does not exist."	
		exit 1
        fi ;;
"start" )
	if [ -f /usr/nserve/rfmaster -a -f /usr/nserve/domain -a -f /usr/nserve/netspec ]; then
		#
		# Start npackd
		#
		if [ $NETSPEC = "npack" ] ; then
			npackd 2>/dev/null &
			if [ $? != 0 ]; then
				ps ax | grep npackd > /tmp/TMP
                		ID=`cat /tmp/TMP | awk '$5 == "npackd" { print $1 }'`
				if [ "$ID" = "" ]; then
                			echo
                			echo "${CMDNAME}: can not start npackd."
					exit 1
				fi
        		fi
		fi

		#
		# Start listener
		#
		rm -f /usr/net/nls/${NETSPEC}/lock
		nlsadmin -s $NETSPEC &
		if [ $? != 0 ]; then
			ps ax | grep listen > /tmp/TMP
                	ID=`cat /tmp/TMP | awk '$5 == "listen" { print $1 }'`
                	if [ "$ID" = "" ]; then
                		echo
                		echo "${CMDNAME}: can not start listener."
				# 
        			# Stop npackd
        			#
				if [ $NETSPEC = "npack" ] ; then
        				ps ax | grep npackd > /tmp/TMP
        				ID=`cat /tmp/TMP | awk '$5 == "npackd" { print $1 }'`
        				kill -9 $ID
        				rm -f /tmp/TMP
                			exit 1
				fi 
			fi
        	fi

		#
		# Start RFS : nserve and fudaemon
		#
		rfstart $VFLAG
		if [ $? != 0 ]; then
                	echo
                	echo "${CMDNAME}: can not start rfstart."
			#  
                	# Stop npackd 
                	#
			if [ $NETSPEC = "npack" ] ; then 
                		ps ax | grep npackd > /tmp/TMP
                		ID=`cat /tmp/TMP | awk '$5 == "npackd" { print $1 }'` 
                		kill -9 $ID
                		rm -f /tmp/TMP
			fi
			#
        		# Stop listener
        		#
        		nlsadmin -k $NETSPEC
                	exit 1 
        	fi

		#
		# Advertise resources in /etc/rstab
		#
		if [ -f /etc/rstab ]; then
			/etc/rstab
		fi
		#
		# Mount resources
		#
		umount -vat rfs > /dev/null 2>&1
		mount -vat rfs -o bg

		echo
		echo "RFS started." 
	else
		echo
                echo "RFS is not initialized and RFS is not started."
	fi
	break ;;
"stop" )
	if [ -f /usr/nserve/rfmaster -a -f /usr/nserve/domain -a \
                        -f /usr/nserve/netspec ]; then
		#
		# Force umount all advertised resources
		#
		if [ -f /etc/advtab ]; then
			for i in `awk '{ print $1 }' < /etc/advtab`; do
				fumount $i
			done
		fi
		#
		# Umount all remotely mounted resources
		#
		mount > /tmp/FSTAB
		for i in `awk '$5 == "rfs" { print $3 }' < /tmp/FSTAB`; do
                	umount -d $i
        	done
		rm -f /tmp/FSTAB
		#
		# Stop RFS : nserve and fudaemon
		#
		rfstop
		if [ $? != 0 ]; then
                	echo
                	echo "${CMDNAME}: can not start rfstop."
                	exit 1
        	fi

		#
		# Stop npackd
		#
		if [ $NETSPEC = "npack" ] ; then
			ps ax | grep npackd > /tmp/TMP
			ID=`cat /tmp/TMP | awk '$5 == "npackd" { print $1 }'`
			kill -9 $ID
			rm -f /tmp/TMP
		fi

		#
		# Stop listener
		#
		nlsadmin -k $NETSPEC
		if [ $? != 0 ]; then
                	echo
                	echo "${CMDNAME}: RFS not stopped."  
                	exit 1
        	fi

		echo
		echo "RFS stopped." 
	else
		echo
                echo "RFS is not initialized and RFS is not running."
	fi
	break ;;
esac
exit 0
