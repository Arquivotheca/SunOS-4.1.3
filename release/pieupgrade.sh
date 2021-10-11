#!/bin/sh
# %Z%%M% %I% %E% SMI
#
# This script will upgrade a machine from the release tar files.
# The tar files *must* already be mounted.
# (The tarfiles for pie can currently
# be mounted from the machine hypersparc.)
#
# The script may be run in single user or multi-user mode.
# It assumes the network can be accessed.
# If it is run in multi-user mode, it is recommended that
# no other users be logged in.  It *must* be run as root. 
# It will now test to see if you are running on the console,
# to attempt to avoid people trashing things when their windows
# system goes bonkers.
# 
# It assumes that all packages listed in /etc/install/media_list 
# are currently installed, and it upgrades these packages only.
# Note that it overwrites existing files and does not remove 
# any obsolete files.
#
# Some files are preserved during the upgrade (eg: fstab,
# passwd, hosts, hosts.equiv, printcap, rc.local, .rhosts).  
# The existing vmunix is saved as vmunix.orig.    
#
# It also can be told to do a particular release,
# otherwise it will use the default release, as found
# in /tarfiles/RELEASES.pieupgrade
#
# New feature:  If there is a file ./pieupgrade.exclude.{root,usr},
# it is concatenated with the built-in list of pieupgrade exclude
# files.  The usr exclude list is used for all non-root categories.
#
# Pieupgrade requires the following statically linked programs 
# to be installed into /tarfiles/bin.`arch`:  pacifier, tee.
#
# Working variables
#
CMDNAME=`basename $0`
ARCH=`/bin/arch -k`
AARCH=`/bin/arch`
TARFILES=/tarfiles
MEDIA_LIST=/etc/install/media_list
HOST=`hostname`
mount=/etc/mount
tmp=/tmp
mail=/usr/ucb/mail
RELEASES=${TARFILES}/RELEASES.pieupgrade
MAIL_LIST="pieupgrade@merp.Eng"
ROOTEXCLUDE="pieupgrade.exclude.root"
USREXCLUDE="pieupgrade.exclude.usr"
TAR=$tmp/tar
SH=$tmp/sh
LOGFILE=/etc/install/pieupgrade.log

# routine to clean up and go away - $1 is the exit code
#
cleanup () {
	rm -f $tmp/*_$$ $tmp/tar
	exit $1
}

#
# Error handling
#
trap 'echo $CMDNAME ABORTED.; cleanup 1' 1 2 3 15

#
# grab argument if there
#
case "$#" in
    0)	Release="";
	;;
    1)	Release="$1";
	;;
    *)	echo "usage: pieupgrade [release]";
	exit 1;
	;;
esac

#
# You must be root to run this script
#
if [ "`whoami`x" != "root"x ]; then
   echo "You must be root to do $CMDNAME!"
   cleanup 1
fi

#
# ARCH must be set
#
if [ "X_${ARCH}" = "X_" -o  "X_${AARCH}" = "X_" ]; then
   echo "${CMDNAME}: arch not set! missing arch command?"
   cleanup 1
fi

#
# You must run this from the "console", so we know you've killed
# the window system.
#
if [ "`tty`x" != "/dev/console"x ]; then
	echo "You must run pieupgrade from the console"
	echo "  so it can be sure that the window system is not running"
	cleanup 1
fi

#
# yes/no responses
#
get_yn='(while read yn; do
        case "$yn" in
                [Yy]* ) exit 0;;
                [Nn]* ) exit 1;;
                    * ) echo -n "Please answer y or n:  ";;
        esac
done)'

#
# Print header
#
echo ""
echo "***** Starting $CMDNAME version %I%  `date` *****"
echo ""
echo "$CMDNAME will perform an upgrade by extracting"
echo "the tar files in /tarfiles/$ARCH and overwriting"
echo "the files on $HOST.  It will only upgrade"
echo "packages which are currently installed on $HOST;"
echo "these are listed in $MEDIA_LIST."
echo ""

# XXX might want to check for a couple of meg free in /tmp

#
# check for if the right /tarfiles mounted, as well as if needing default
if [ ! -f $RELEASES ]; then
	echo "$CMDNAME:  Unable to find RELEASES file \" $RELEASES\"."
	cleanup 1
fi

# check for default, else check for valid release
if [ "${Release}"X = "X" ]; then
	Release=`grep "${ARCH}[	 ]*" ${RELEASES} | ( read a rel; echo $rel )`
	if [ "${Release}"X = "X" ]; then
		echo "$CMDNAME:  no default release found for $ARCH in \" $RELEASES\"."
		cleanup 1
	fi
fi

TARPATH="${TARFILES}/${ARCH}_${Release}"

#
# check for validity of Release
if grep -s "${AARCH}.${ARCH}.sunos.${Release}" ${TARFILES}/avail_arches ; then
	# a valid release, no need to do anything
	:
else
	echo "$CMDNAME: release \"${Release}\" not available"
	# print available releases for this arch
	echo "  The available releases for \"${ARCH}\" are:"
	grep "\.${ARCH}\." ${TARFILES}/avail_arches | sed "s/.*sunos\.//"
	cleanup 1 
fi

echo "$CMDNAME: upgrading to release \"${Release}\" for arch: \"${ARCH}\""

# blasted HSFS names have no ".", all underscores!
_release=`echo ${Release} | tr "." "_" | tr '[A-Z]' '[a-z]'`

#
# Check for existence of media_list
# Check for existence of packages listed in media_list in /tarfiles
#
# XXX if media_list older than media_file, need to re-build it too.
#
if [ ! -f $MEDIA_LIST ]; then
	# media_list only done if suninstall, not for quickinstall
	# try to create it from media_file
	oldIFS=${IFS}    
	media_files="/etc/install/media_file.${AARCH}.${ARCH}.sunos.4.1*" 
	media_file=`echo $media_files | ( read foo junk; echo $foo )` 
	IFS="=${IFS}"
	cat $media_file | egrep 'mf_name=|mf_loaded=' | \
	while read nkey name
	do
		read lkey loaded
		if [ ${loaded} '=' "yes" ]; then
			echo $name >> $MEDIA_LIST
		fi
	done
	IFS=${oldIFS}
	#
	if [ ! -f $MEDIA_LIST ]; then
		echo "$CMDNAME:  Can't find or create $MEDIA_LIST file."
		cleanup 1
	fi
fi
if [ ! -s $MEDIA_LIST ]; then
	echo "$CMDNAME:  $MEDIA_LIST file is empty."
	cleanup 1
fi

#
# check availability, first translate a package name into a path
# also squirrel away commands to do the actual upgrade into a
# shell script in /tmp
#
echo '#!/tmp/sh' > $tmp/pieupgrade_$$
# XXX anything else to start?

echo "currently installed on $HOST"
echo "----------------------------"
while read package junk
do
	lcp=`echo "${package}" | tr '[A-Z]' '[a-z]'`
	case "${package}" in
	"root")
		dir="/"
		exc="$tmp/EXCLUDE_$$"
		Xflg="X"
		ppath=${TARFILES}/export/exec/proto_root_sunos_${_release}
		;;
	"Kvm"|"Sys")
		dir="/usr/kvm"
		exc=""
		Xflg=""
		ppath=${TARFILES}/export/exec/kvm/${ARCH}_sunos_${_release}/$lcp
		;;
	"Manual")
		if [ -w /usr/share -a -w /usr/share/man ]; then
			dir="/usr"
			exc="$tmp/usrEXCLUDE_$$"
			Xflg="X"
			ppath=${TARFILES}/export/share/sunos_${_release}/$lcp
		else
			echo "/usr/share/man not writeable, skipping \"Manual\""
			continue;
		fi
		;;
	*)	# everything else is aarch
		dir="/usr"
		exc="$tmp/usrEXCLUDE_$$"
		Xflg="X"
		ppath=${TARFILES}/export/exec/${AARCH}_sunos_${_release}/$lcp
		;;
	esac
	if [ ! -f ${ppath} ]; then
		echo "$CMDNAME: tarfiles for \"${package}\" not available!"
		cleanup 1
	fi
	echo "cd $dir" >> $tmp/pieupgrade_$$
	echo "echo Extracting files for \'$package\' from \'${ppath}\'" >> $tmp/pieupgrade_$$
	echo "/tarfiles/bin.$AARCH/pacifier ${ppath} | $TAR xBpf${Xflg} - ${exc}" >> $tmp/pieupgrade_$$
	echo "$package"
done < $MEDIA_LIST

# NOTE: $tmp/pieupgrade_$$ is finished out down below

echo ""
echo -n "Continue (y/n) ? "
eval "$get_yn"
if [ $? -ne 0 ]; then
	cleanup 1 
fi
echo ""

#
# Make sure /usr is mounted read-write
#
echo "Checking for writability of /usr partition..."
$mount | fgrep ' /usr ' | fgrep -s rw 
if [ $? -ne 0 ]; then
	echo "Remounting /usr read-write..."
	$mount -o rw,remount /usr
	if [ $? -ne 0 ]; then
		echo "$CMDNAME:  Unable to do remount!"
	        cleanup 1
	fi
fi

#
# Exclude important files 
#
# NOTE THAT root .cshrc and .login are NOT preserved
#
echo "./.rhosts"          > $tmp/EXCLUDE_$$
echo "./etc/inetd.conf"  >> $tmp/EXCLUDE_$$
echo "./etc/aliases"     >> $tmp/EXCLUDE_$$
echo "./etc/aliases.dir" >> $tmp/EXCLUDE_$$
echo "./etc/aliases.pag" >> $tmp/EXCLUDE_$$
echo "./etc/dumpdates"   >> $tmp/EXCLUDE_$$
echo "./etc/fstab"       >> $tmp/EXCLUDE_$$
echo "./etc/group"       >> $tmp/EXCLUDE_$$
echo "./etc/passwd"      >> $tmp/EXCLUDE_$$
echo "./etc/hosts"       >> $tmp/EXCLUDE_$$
echo "./etc/hosts.equiv" >> $tmp/EXCLUDE_$$
echo "./etc/printcap"    >> $tmp/EXCLUDE_$$
echo "./etc/rmtab"       >> $tmp/EXCLUDE_$$
echo "./etc/syslog.conf" >> $tmp/EXCLUDE_$$
echo "./etc/mtab"        >> $tmp/EXCLUDE_$$
echo "./etc/rc.local"    >> $tmp/EXCLUDE_$$
echo "./etc/xtab"        >> $tmp/EXCLUDE_$$
echo "./etc/utmp"        >> $tmp/EXCLUDE_$$
echo "./var/spool/cron/crontabs/root"        >> $tmp/EXCLUDE_$$
echo "./var/spool/cron/crontabs/at.deny"     >> $tmp/EXCLUDE_$$
echo "./var/spool/cron/crontabs/cron.deny"   >> $tmp/EXCLUDE_$$
echo "./home"            >> $tmp/EXCLUDE_$$

# in /usr
echo "./share/lib/zoneinfo/localtime" >> $tmp/usrEXCLUDE_$$

# if there are separate root or usr exclude lists, 
# concatenate them to the built-in lists
#
[ -f ./$ROOTEXCLUDE ] && cat $ROOTEXCLUDE >> $tmp/EXCLUDE_$$
[ -f ./$USREXCLUDE ] && cat $USREXCLUDE >> $tmp/usrEXCLUDE_$$
 
echo "Preserved in root during upgrade:"
cat $tmp/EXCLUDE_$$
echo ""
echo "Preserved in /usr during upgrade:"
cat $tmp/usrEXCLUDE_$$

echo ""
echo -n "Start installation (y/n) ? "
eval "$get_yn"
if [ $? -ne 0 ]; then
	cleanup 1 
fi
echo ""

# HARDEN this process, but copying the shell (statically linked)
# to /tmp, then copying the
cp -p /bin/tar $TAR
cp -p /sbin/sh $SH

cat << EOF >> $tmp/pieupgrade_$$
#
# routine to send a log message to the mailing list
#
maillog () {
	echo "$LOGNAME just completed installing $ARCH $Release on host $HOST" \
	| $mail -s "$CMDNAME LOG" $MAIL_LIST
}

#
# Copy and verify the kernel.
# If vmunix.orig exists, do not save the existing version.
# If copy operation fails, try one more time.
#
echo "Installing vmunix..."
FROM=/usr/kvm/stand
if [ ! -s \$FROM/vmunix ]; then
	echo "Can't find vmunix in \$FROM!"
else 
	if [ -f /vmunix ]; then
       		 [ ! -s /vmunix.orig ] && mv /vmunix /vmunix.orig
	fi

	for i in 1 2; do
       		cp -p \$FROM/vmunix /vmunix
        	sync; sync
        	sum1=\`/usr/bin/sum \$FROM/vmunix\`
        	sum2=\`/usr/bin/sum /vmunix\`
       	 	[ "\$sum1" = "\$sum2" ] && break
        	if [ "\$i" = 2 ]; then
	        	echo "Unable to install new kernel for $HOST!"
                	[ ! -f /vmunix.orig ] && break
                	echo "Restoring original kernel..."
                	mv /vmunix.orig /vmunix
                	sync; sync
        	fi
	done
fi

echo "Installing kadb..."
if [ ! -s \$FROM/kadb ]; then
	echo "Can't find kadb in \$FROM!"
else
	cp -p \$FROM/kadb /kadb
	sync; sync
fi

#
# Install boot block
#
DISK=`mount | grep ' / ' | awk '{print $1}' | awk -F/ '{print $3}'`
DISKTYPE=\`echo \$DISK | colrm 3\`
echo "Installing bootblock..."
cp -p /usr/kvm/stand/boot.$ARCH /boot
sync; sync
cd /usr/mdec
installboot -v /boot boot"\${DISKTYPE}" /dev/r"\${DISK}"
sync; sync

#
# Install sbin
#
cd /usr
cp -p bin/hostname /sbin
cp -p etc/ifconfig /sbin
cp -p etc/init     /sbin
cp -p etc/mount    /sbin
cp -p etc/intr     /sbin
cp -p bin/sh       /sbin
sync; sync

#
# Finish up
#
maillog
echo ""
echo "You may reboot $HOST now."
echo ""
echo "***** Exiting $CMDNAME version %I%  \`date\` *****"
EOF

echo "Output logged to $LOGFILE"
echo 
trap '' 1 2 3 15
nohup $SH /tmp/pieupgrade_$$ | /tarfiles/bin.$AARCH/tee /dev/console >> $LOGFILE
wait
