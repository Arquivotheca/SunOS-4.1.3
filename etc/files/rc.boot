#! /bin/sh -
#
#       %Z%%M% %I% %E% SMI
#
# Executed once at boot time
#
PATH=/sbin:/single:/usr/bin:/usr/etc; export PATH
HOME=/; export HOME
#
# It is important to fsck the root filesystem here
# to prevent spurious panics after system crashes.
#
# In SunOS 4.x, block devices are no longer consistent with file systems
# mounted read-write.  To allow file systems to be reliably fsck'ed during
# system reboots, the root 4.2 (UFS) file system is by default mounted
# read-only.  Fsck uses the raw device to check for any file system damage.
# This way fsck can repair any file system damage without the kernel
# overwriting it with any potentially corrupt in-core file system control
# information it might have.  The file systems will remain mounted
# read-only until a "remount" operation by /etc/rc.single is done after
# the disks check out cleanly.  Thus if fsck finds and repairs a corrupt
# file system which is currently mounted the system should be rebooted
# immediately.
#
# Simulate cat in sh so we don't need to have it on the root filesystem
#
shcat() {
	while test $# -ge 1
	do
		while read i
		do
			echo "$i"
		done < $1
		shift
	done
}

#
# Set hostname from /etc/hostname.xx0 file, if none exists no harm done
#

hostname="`shcat /etc/hostname.??0			2>/dev/null`"
if [ ! -f /etc/.UNCONFIGURED -a ! -z "$hostname" -a "$hostname" != "noname" ]; then
 	hostname $hostname
fi

#
# Get the list of ether devices to ifconfig by breaking /etc/hostname.* into
# separate args by using "." as a shell separator character, then step
# through args and ifconfig every other arg.
#
interface_names="`shcat /etc/hostname.*			2>/dev/null`"
if test -n "$interface_names"
then
	(
		IFS="$IFS."
		set `echo /etc/hostname\.*`
		while test $# -ge 2
		do
			shift
			if [ "$1" != "xx0" ]; then
				ifconfig $1 "`shcat /etc/hostname\.$1`" netmask + -trailers up 
			fi
			shift
		done
	)
fi

#
# configure the rest of the interfaces automatically, quietly.
#
ifconfig -ad auto-revarp up

# 
# set host info from bootparams if not locally configured
#
if [ -z "`hostname`" ]; then
	hostconfig -p bootparams
fi

# 
# If local and network configuration failed, re-try forever in an
# interruptable sub-shell.  We want this sub-shell to be interruptable
# so that the machine can still be brought up manually when the servers
# are not cooperating.
#

if [ -z "`hostname`" -a ! -f /etc/.UNCONFIGURED ]; then
	echo "host configuration failed - re-trying..."
	intr sh /etc/rc.ip
fi
	
ifconfig lo0 127.0.0.1 up

#
# If "/usr" is going to be NFS mounted from a host on a different
# network, we must have a routing table entry before the mount is
# attempted.  One may be added by the diskless kernel or by the
# "hostconfig" program above.  Setting a default router here is a problem
# because the default system configuration does not include the
# "route" program in "/sbin".  Thus we only try to add a default route
# at this point if someone managed to place a static version of "route" into
# "/sbin".  Otherwise, we add the route in "/etc/rc.local" after "/usr" 
# has been mounted and NIS is running.
#
# Note that since NIS is not running at this point, the router's name 
# must be in "/etc/hosts" or its numeric IP address must be used in the file.
# 
if [ -f /sbin/route -a -f /etc/defaultrouter ]; then
	route -f add default `cat /etc/defaultrouter` 1
fi

unset shcat

#
# This function is used when fsck detects an error and we want to skip
# remounting the file systems read-write to facilitate any needed repair.
# The root 4.2 file system is always mounted read-only during booting
# unless the "-w" flag is given when booting the file system.
#
noremount() {
	echo ""
	echo "WARNING - file systems have NOT been remounted read-write."
	echo "Use fsck to fix any file system problems, rebooting the"
	echo "system if any problems are found with a mounted file system."
	echo "After file systems have fsck'ed cleanly, you can remount file"
	echo "systems and finish single-user setup using \"/etc/rc.single\"."
}

intr mount -n -r /usr

# trust that everything is ok when /fastboot exists
# and skip doing the fsck'ing the file systems.
if [ -r /fastboot ]; then
	echo "Fast boot ... skipping disk checks"
	error=0
else
	if [ $1x = singleuserx ]; then
		echo "checking / and /usr filesystems"
		intr fsck -p -w / /usr
		error=$?
		what="Fsck"
	else
		echo "checking filesystems"
		intr fsck -p -w
		error=$?
		what="Reboot"
	fi
fi

case $error in
0|2)
	#
	# Everything looks good.
	#
	# Finish the single user setup which will remount the
	# file systems read-write and do other work which can
	# be done only on a writable root file system.
	#
	sh /etc/rc.single
	#
	# We need to check whether rc.single successfully completed.
	# It can fail, for example, when /etc/fstab has gotten fouled up.
	#
	nerr=$?
	case $nerr in
	0)
		error=$nerr
		;;
	1)
		echo "Remount of / failed - check /etc/fstab"
		error=$nerr
		;;
	2)
		echo "Remount of /usr failed - check /etc/fstab"
		error=$nerr
		;;
	*)
		echo "Unknown error in /etc/rc.single - help!"
		error=$nerr
		;;
	esac
	;;
4)
	if [ $1x = singleuserx ]; then
		echo "Mounted FS fixed - rebooting single-user."
		reboot -q -n -- -s
	else
		echo "Mounted FS fixed - rebooting."
		reboot -q -n
	fi
	;;
8)
	echo "$what failed...help!"
	noremount
	;;
12)
	echo "$what interrupted."
	noremount
	;;
*)
	echo "Unknown error in reboot fsck."
	noremount
	;;
esac

#
# exit with error status from fsck
#
exit $error
