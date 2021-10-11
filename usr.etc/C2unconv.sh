#! /bin/sh
#
#  Copyright (c) 1987 by Sun Microsystems, Inc.
#	%Z%%M% %I% %E% SMI
#
umask 022
passwd=/etc/passwd
passwda=/etc/security/passwd.adjunct
group=/etc/group
groupa=/etc/security/group.adjunct

if [ ! -r $passwd ]
then
	echo "Missing file $passwd"
	exit 1
fi
if [ ! -r $passwda ]
then
	echo "Missing file $passwda"
	exit 1
fi
if [ ! -r $group ]
then
	echo "Missing file $group"
	exit 1
fi
if [ ! -r $groupa ]
then
	echo "Missing file $groupa"
	exit 1
fi

tfpx=/tmp/C2.$$px
tfpout=/tmp/C2.$$pout
echo "Converting $passwd file"

# Generate sed script to put passwords back in passwd file
sed -e 's,^\([^:]*\):\([^:]*\).*$,s:##\1\\::\2\\::,' <$passwda | grep '^s:' >$tfpx

# Run sed script to generate passwd file with passwords
sed -f $tfpx <$passwd >$tfpout

# Make backups
/bin/mv -f $passwd $passwd.bak
/bin/mv -f $passwda $passwda.bak
# Install new passwd file
/bin/mv -f $tfpout $passwd
/bin/rm -f $tfpx $tfpout

tfgx=/tmp/C2.$$gx
tfgout=/tmp/C2.$$gout
echo "Converting $group file"

# Generate sed script to put passwords back in group file
sed -e 's,^\([^:]*\):\([^:]*\).*$,s:#$\1\\::\2\\::,' <$groupa | grep '^s:' >$tfgx

# Run sed script to generate passwd file with passwords
sed -f $tfgx <$group >$tfgout

# Make backups
/bin/mv -f $group $group.bak
/bin/mv -f $groupa $groupa.bak
# Install new group file
/bin/mv -f $tfgout $group
/bin/rm -f $tfgx $tfgout
