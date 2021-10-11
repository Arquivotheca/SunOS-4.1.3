#! /bin/sh
#
#	%Z%%M% %I% %E% SMI
#
#  Copyright (c) 1987 by Sun Microsystems, Inc.
#
# Script to convert a SunOS system to C2 security.
#
BASE=""
EXECS="/usr"
ROOTS=""
CLIENTS=""
DEVICES=""
DIRECTORIES=""
FILESYSTEMS=""
OPTIONS=""
FLAGS="ad,lo,p0,p1"
MINFREE="20"
HOSTNAME=`hostname`
SYSADMIN=root@$HOSTNAME

#
# Should only be run by root
#
name=`whoami`
case "$name" in
"root" )
	break;;
* )
	echo "C2conv can only be run by root."
	exit 1
	break;;
esac
#
# Introduction
#
echo "You are about to run the C2conv program. Please read the C2 security"
echo "chapter in the System and Network Administration Manual if you have not"
echo "done so yet. This program generates a shell script before it actually"
echo "affects any files. You may cancel the installation by entering Control-C"
echo "at any prompt. At the end of the procedure, you will receive a final"
echo "confirmation before applying the changes. You may then abort the"
echo "procedure and examine the generated files."
echo ""
#
# Should be run single-user
#
while true; do
	echo -n "Is the system in single-user mode? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
#
# local audit and export filesystems must be mounted
#
		mount -at 4.2
		break;;
	"n" | "no" )
		echo "Aborting conversion."
		echo "C2conv must be run in single-user mode."
		exit 1
		break;;
	esac
done
#
# Non-root base can be specified in 2nd argument
#
if test $2
then
	BASE=$2
fi
#
# Get mount options
#
while true; do
	echo -n "Do you want audit file systems mounted using Secure NFS [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		OPTIONS="secure"
		break;;
	"n" | "no" )
		break;;
	esac
done
#
# Get the location of the clients' roots
#
while true; do
	echo ""
	echo -n "Is $HOSTNAME a server for diskless clients? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		while true; do
		echo -n "    Enter path of clients' root (e.g. '/export/root'): "
		read ROOTS;
		if [ -d "$BASE""$ROOTS" ]
		then
			CLIENTS=`ls "$BASE""$ROOTS" | sed -e '/lost+found/d'`
			CLIENTS=`echo $CLIENTS | sed -e 's/  */,/g'`
				break
			else
				echo "'$BASE$ROOTS': no such directory"
		fi
		done
		while true; do
			echo "    Enter path of additional architecture's executables "
			echo -n "        (e.g. '/export/execs/sun3') or 'done': "
			read RESPONSE;
			case "$RESPONSE" in
			"/usr" )
				;;
			"done" )
				break ;;
			* )
				if [ ! -d "$RESPONSE" ]
				then
					echo "'$RESPONSE': no such directory"
					continue
				fi
				EXECS="$EXECS","$RESPONSE" ;;
			esac
		done
		break ;;
	"n" | "no" )
		ROOTS=""
		break ;;
	esac
done

#
# Set up the default list of audit clients. Assume that the local set
# is it.
#
echo ""
while true; do
	echo -n "Is $HOSTNAME an audit file server? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		while true; do
			echo -n "    Enter audit device (e.g. 'xy1d'), or 'done': "
			read RESPONSE;
			case "$RESPONSE" in
			"done" )
				break ;;
			* )
				DEVICES="$DEVICES","$RESPONSE" ;;
			esac
		done
		
		break ;;
	"n" | "no" )
		DEVICES=""
		break ;;
	esac
done
#
# Names of audit servers and file systems
#
echo ""
while true; do
	echo -n "Enter name of remote audit file server, or 'done': "
	read RESPONSE;
	case "$RESPONSE" in
	"done" )
		break ;;
	"" )
		continue ;;
	* )

		while true; do
			echo "    Enter remote audit file system on $RESPONSE"
			echo -n "    (e.g. '/etc/security/audit/$RESPONSE'), or 'done': "
			read RESPONSE1;
			case "$RESPONSE1" in
			"done" )
				break ;;
			* )

				FILESYSTEMS="$FILESYSTEMS","$RESPONSE":"$RESPONSE1" ;;
			esac
		done
		;;
	esac
done
#
# Names of other directories to audit to
#
echo ""
while true; do
	echo -n "Specify other audit directories or 'done': "
	read RESPONSE
	case "$RESPONSE" in
	"done" )
		break ;;
	"" )
		continue ;;
	* )
		DIRECTORIES="$DIRECTORIES","$RESPONSE" ;;
	esac
done
	if [ -n "$DIRECTORIES" -a -n "$CLIENTS" ]
	    then
		echo ""
		echo "Notice: Clients may require mounts for these directories."
	    fi
#
# Get audit flags
#
echo ""
echo "You are about to be asked to set the audit flags."
echo -n "Do you need a summary? [y|n]: "
read RESPONSE;
case "$RESPONSE" in
"y" | "yes" )
    echo ""
    echo "The audit flags specify which event classes are to be audited."
    echo "Each flag identifies a single audit class. To specify more"
    echo "than one flag, enter them as a comma-separated list. No spaces"
    echo "are allowed in your answer."
    echo ""
    echo "The following table lists the audit classes:"
    echo ""
    echo "    flag name               short description"
    echo ""
    echo "       dr                Read of data, open for reading, etc."
    echo "       dw                Write or modification of data"
    echo "       dc                Creation or deletion of any object"
    echo "       da                Change in object access (modes, owner)"
    echo "       lo                Login, logout, creation by at(1)"
    echo "       ad                Normal administrative operation"
    echo "       p0                Privileged operation"
    echo "       p1                Unusual privileged operation"
    echo ""
    echo "The following prefixes may be used as options:"
    echo ""
    echo "   -        audit for failure only"
    echo "   +        audit for success only"
    echo "(no prefix) audit for both successes and failures "
    echo ""
    echo "The default audit flags are '$FLAGS'"
    echo ""
	break;;
"n" | "no" )
	break;;
esac

while true; do
	echo -n "OK to use audit flags '$FLAGS'? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		break;;
	"n" | "no" )
		echo -n "Enter audit flags (e.g. '-ad,p0,+lo'): "
		read FLAGS;
		FLAGS=`echo $FLAGS | sed 's/ //g'`
		break;;
	esac
done
#
# Get minfree value
#
while true; do
	echo -n "OK to use soft disk space limit of 20%? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		break;;
	"n" | "no" )
		echo -n "Enter soft limit percentage (e.g. '10'): "
		read MINFREE;
		break;;
	esac
done
#
# Get notification list
#
while true; do
	echo -n "OK to notify '$SYSADMIN' when administration is required? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		break;;
	"n" | "no" )
		echo -n "Enter notification address (e.g. 'joe@capitale'): "
		read SYSADMIN;
		break;;
	esac
done
#
#
#
rm -f C2conv_input C2conv_script

echo "base=$BASE" >> C2conv_input
echo "flags=$FLAGS" >> C2conv_input
echo "minfree=$MINFREE" >> C2conv_input
echo "sysadmin=$SYSADMIN" >> C2conv_input

echo "execs=`echo "$EXECS" | sed -e 's/^,//'`" >> C2conv_input
echo "roots=`echo "$ROOTS" | sed -e 's/^,//'`" >> C2conv_input
echo "clients=`echo "$CLIENTS" | sed -e 's/^,//'`" >> C2conv_input
echo "devices=`echo "$DEVICES" | sed -e 's/^,//'`" >> C2conv_input
echo "filesystems=`echo "$FILESYSTEMS" | sed -e 's/^,//'`" >> C2conv_input
echo "directories=`echo "$DIRECTORIES" | sed -e 's/^,//'`" >> C2conv_input
echo "options=`echo "$OPTIONS" | sed -e 's/^,//'`" >> C2conv_input

/usr/lib/c2convert < C2conv_input > C2conv_script
#
#
# Offer escape
#
while true; do
	echo -n "Last chance to abort gracefully." \
	    " Do you want to continue? [y|n]: "
	read RESPONSE;
	case "$RESPONSE" in
	"y" | "yes" )
		sh C2conv_script
		rm C2conv_script C2conv_input
#
# Set password for "audit" id
#
		echo -n "Do you want to set a local password for 'audit'? [y|n]: "
		read RESPONSE;
		case "$RESPONSE" in
		"y" | "yes" )
			while true; do
				echo "Setting password for 'audit' ..."
				passwd audit
				if test $? -eq 1
				then
				:
				else
					break
				fi
			done
			break;;
		"n" | "no" )
			break;;
		esac
		echo "Some additional file systems may now be mounted."
		break;;
	"n" | "no" )
		echo "Aborting conversion."
		echo "  Replies left in 'C2conv_input'.  Script left in 'C2conv_script'."
		echo "Some additional file systems may now be mounted."
		exit 1
		break;;
	esac
done
