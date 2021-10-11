#
# designed to be sourced by munixfs .profile (nee Install.miniroot.sh)
# requires that "disk" be set to sd0, sd1, etc.
{
	device=/dev/rfd0c
	echo -n "insert diskette \"C\", press return when ready" > /dev/tty;
	read answer 2>&1 > /dev/tty;
	fdrolloff "C" $device
	eject 2> /dev/tty;
	echo -n "insert diskette \"D\", press return when ready" > /dev/tty;
	read answer 2>&1 > /dev/tty;
	fdrolloff "D" $device
	eject 2> /dev/tty;
	echo -n "insert diskette \"E\", press return when ready" > /dev/tty;
	read answer 2>&1 > /dev/tty;
	fdrolloff "E" $device
	eject 2> /dev/tty;
} | uncompress -c > /dev/r${disk}b 2> /dev/tty
