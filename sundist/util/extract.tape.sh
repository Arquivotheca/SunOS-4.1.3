#
# @(#)extract.tape.sh 1.1 92/07/30 Copyright 1989 Sun Microsystems
# Designed to be sourced by /bin/sh
# This extracts miniroot from tape and stick it on disk partition "b"
#
# INPUTS: set variable "disk" to be disk name of form sd0, sd1, etc.
# Unix name of device munixfs is loaded from is conveniently stuck
# in the first block of the ramdisk
#
# NOTE: someone decided to link "mt" to "st" !?!?!?
#
loadfrom=`dd if=/dev/rd0a bs=1b count=1 2>/dev/null | ( read aword junk; echo $aword )`
dev=`expr $loadfrom : '\([A-Za-z]*\)'`
unit=`expr $loadfrom : '[A-Za-z]*\([0-9]*\)'`
if [ "X${unit}" = "X" ] ; then
	unit="0";
fi
if [ "${dev}" = "mt" -o "${dev}" = "xt" ]; then
	# mt is probably linked to st
	echo making device nodes for ${dev}${unit}
	( cd /dev; MAKEDEV ${dev}${unit} )
	dev="mt"
fi
tape="${dev}${unit}"
# KLUDGE for sun3e - no auto density sense, force to st8
if sun3 && [ ${dev} = "st" ]; then
	case `hostid` in
	18*)	unit=`expr $unit + 8`;
		tape="${dev}${unit}";;
	esac
fi
		
#
# try to rewind the tape as a probe as well as rewind
while :
do
	mt -f /dev/r${tape} rewind
	if [ $? -eq 0 ]
	then
		break;
	fi
	echo "Problem with tape: what do you want to do?"
	echo "  1 - retry the tape \"${tape}\""
	echo "  2 - use a different tape unit"
	echo "  3 - abandon miniroot install and enter single user shell"
	echo -n "Enter a 1, 2 or 3: "
	read answer
	case "$answer" in
	    1) echo ""; echo "retrying..."; continue ;;
	    2) echo -n "enter tape unit, in format of \"st0\" or \"mt0\": " ;
		read tape ;
		continue ;;
	    3) echo "$RestartMsg"; break 2 ;;
	    *) echo "INVALID RESPONSE" ;;
	esac
done
#
echo "using tape \"${tape}\""
#
# Begin extracting the miniroot from tape.
#
echo "Extracting miniroot (takes a couple of minutes) ..."
if sun4c || sun4m ; then
	mt -f /dev/nr${tape} fsf 2
else
	mt -f /dev/nr${tape} fsf 4
fi
if [ $? -ne 0 ] ; then
	echo "ERROR skipping to miniroot file on tape: /dev/nr${tape}"
	break 2;
fi
# HARDCODE: FSBLOCKING is 100k XXX - ought to pass in, but it is "standard"
dd if=/dev/nr${tape} of=/dev/r${disk}b bs=100k
if [ $? -ne 0 ] ; then
	echo "ERROR loading miniroot from: /dev/nr${tape} to: /dev/r${disk}b"
	break 2;
fi
#
