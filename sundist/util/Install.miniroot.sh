#
# @(#)Install.miniroot.sh 1.1 92/07/30 Copyright 1989 Sun Microsystems
#
# generic script to install a miniroot from munixfs
#  appended to .profile
#  assumes prepended (in .profile) is:
#  1. something to setup any arguments needed by "/extract"
#  2. any specific initialization - ie. eject the floppy
#  assumes there exists a file "disks.known" - a list of known disks
#  assumes that there exists a script "/extract" which expects $disk to be
#	setup, and sourceing "/extract" will load the miniroot onto the disk
#	"disk" is "sd0", "sd1", etc.
#
#
HOME=/
export HOME
umask 022
RestartMsg='you may restart this script by typing <cntl-D>'
#
# outer layer while - just because profile is source by sh, so we can't exit!
while :
do
# turn off traps in first part - must exit in orderly fashion 
trap '' 1 2 15		# TRAP OFF
#
# Give the user their options.
#
while :
do
    echo
    echo "What would you like to do?"
    echo "  1 - install SunOS mini-root"
    echo "  2 - exit to single user shell"
    echo -n "Enter a 1 or 2: "
    read answer
    case "$answer" in
	1) break ;;
	2) echo $RestartMsg; trap 1 2 15 ; break 2 ;;
	*) echo "INVALID RESPONSE" ;;
    esac
done
#
echo "Beginning system installation - probing for disks."
#
# use format to probe for disks
echo '0\nquit' | format > disks.tmp 2> /dev/null
# toss 1st 4 lines, toss last line, toss big whitespace, toss everything
#  from "cyl" on over on second lines of output from format(8)
# 2x w in case little space remains, eh sun3e?
cat << XXX | ed -s disks.tmp
1,4d
\$d
1,\$s/[ 	]*//
1,\$s/ cyl [0-9][0-9]*.*\$//
w
w
q
XXX
#
# the ed step should leave you with something like...
# 0. sd0 at esp0 slave 24
#    sd0: <Quantum ProDrive 105S
# 1. sd1 at esp0 slave 8
#    sd1: <Quantum ProDrive 105S
#
# these lines are joined and parsed with shell read...
#
cat /dev/null > disks.here
while read number name w1 w2 w3 w4
do
	read namecolon maker model
	echo $name "${maker} ${model}>" $w1 $w2 $w3 $w4 >> disks.here
done < disks.tmp

# put just the disk names into the variable "disklist"
disklist=` while read disk comments
do
	echo -n "${disk} "
done < disks.here
`
# now add up how many disks there are
ndisks="0"
for disk in ${disklist}
do
	ndisks=`expr ${ndisks} + 1`
done
#
# now display a menu - only if more than one disk present!
#
if [ ${ndisks} -eq 0 ]
then
	echo "Error: no disks were found by format(8) program"
	echo ""
	echo "exiting to single user shell"
	echo "\"cat README\" for manual miniroot installation instructions"
	trap 1 2 15
	break;
elif [ ${ndisks} -eq 1 ]
then
	disk=` echo ${disklist}`	#toss leading,trailing spaces
	echo "installing miniroot on disk \"${disk}\", (only disk found)."
else
	# we pray that the Bourne shell had arrays
    while :
    do
	echo "Which disk do you want to be your miniroot system disk?"
	cnt=1
	for disk in ${disklist}
	do
		echo -n "  ${cnt} - ${disk}: "
		echo "/^${disk}/s/${disk}//p" | ed -s disks.here
		cnt=`expr $cnt + 1`
	done
	echo "  ${cnt} - exit to single user shell"
	#
	# make a pretty prompt - "Enter a 1, 2 or 3: ".
	# be sure to handle "Enter a 1 or 2: " case correctly.
	#
	tmp_cnt=1
	echo -n "Enter a ${tmp_cnt}"
	for disk in ${disklist}
	do
		tmp_cnt=`expr $tmp_cnt + 1`
		if [ ${tmp_cnt} -eq ${cnt} ]
		then
			echo -n " or ${tmp_cnt}: "
		else
			echo -n ", ${tmp_cnt}"
		fi
	done
	read answer
	if [ $answer -eq $cnt ]
	then
		# selected "exit to single user shell"
		echo $RestartMsg
		trap 1 2 15
		break 2;
	elif [ $answer -ge 1 -a $answer -lt $cnt ]
	then
		# an answer in valid range
		cnt=1
		for disk in ${disklist}
		do
			if [ $cnt -eq $answer ]
			then
				echo "selected disk unit \"${disk}\"."
				break 2;
			fi
			cnt=`expr $cnt + 1`
		done
	else
		echo "INVALID RESPONSE"
	fi
    done
fi
trap 1 2 15
#
# give user a chance to format it
# NOTE: we disable our traps so someone can ^C in format and not kill us too
# XXX would be nice to give instructions on how to run "format", maybe we
# XXX should have a canned input and a manual format choice
#
while :
do
	echo "Do you want to format and/or label disk \"${disk}\"?"
	echo "  1 - yes, run format"
	echo "  2 - no, continue with loading miniroot"
	echo "  3 - no, exit to single user shell"
	echo -n "Enter a 1, 2, or 3: "
	read answer
	case "$answer" in
	    1) trap '' 1 2 15; format -d ${disk} ; trap 1 2 15 ; break ;;
	    2) break ;;
	    3) echo $RestartMsg; break 2 ;;
	    *) echo "INVALID RESPONSE" ;;
	esac
done
#
if [ ! -c /dev/r${disk}b ]
then
	echo "Error: the file \"/dev/r${disk}b\" does not exist or is not a character device"
	echo "exiting to single user shell"
	break
fi
echo "checking writeability of /dev/r${disk}b"
echo "test" | dd of=/dev/r${disk}b bs=1b conv=sync >/dev/null
if [ $? -ne 0 ]
then
	echo "Error: could not open or write to disk \"/dev/r${disk}b\""
	echo "exiting to single user shell"
	break
fi
#
# Begin extracting the miniroot from diskette.
#
echo "Extracting miniroot ..."
. /extract
#
#
# Finish up.
#
sync ; sync ; sync
echo
echo "Mini-root installation complete."
#
# FRIENDLINESS
bootunit=`expr $disk : '[a-zA-Z][a-zA-Z]*\([0-9][0-9]*\)' `
bootdev=`expr $disk : '\([a-zA-Z][a-zA-Z]*\)[0-9][0-9]*' `
# the old proms have interesting names for SCSI disks
# should really be ifndef OPENPROM, but this is the shell...
if sun4c || sun4m; then
	bootargs=`/usr/kvm/unixname2bootname /dev/${disk}b`
	bootargs="$bootargs -sw"
elif [ $bootdev = "sd" ] && sun3 || sun4 || sun3x ; then
	# unit = (unit & 0xFE << 2) | (unit & 0x1)
	r=`expr $bootunit % 2`
	u=`expr $bootunit / 2`
	u=`expr $u \* 8`
	bootunit=`expr $u + $r`
	# ARGH!, unit is in hex!, but saved somewhat by multiples of 8
	# with maybe +1, so it is 0, 1, 8, 9, 0x10, 0x18, thus no hex letters
	if [ $bootunit -gt 10 ]; then
		u=`expr $bootunit / 16`
		r=`expr $bootunit % 16`
		bootunit=`expr '(' $u \* 10 ')' + $r`
	fi
	bootargs="${bootdev}(0,${bootunit},1) -sw"
else
	bootargs="${bootdev}(0,${bootunit},1) -sw"
fi
#
while :
do
    echo
    echo "What would you like to do?"
    echo "  1 - reboot using the just-installed miniroot"
    echo "  2 - exit into single user shell"
    echo -n "Enter a 1 or 2: "
    read answer
    case "$answer" in
	1) echo "rebooting from: $bootargs";
	   reboot -q "$bootargs" ;;
	2) echo $RestartMsg;
	   echo "you may boot the miniroot by typing: reboot -q '$bootargs'";
	   break 2 ;;
	*) echo "INVALID RESPONSE" ;;
    esac
done
break
# end outer layer while
done
