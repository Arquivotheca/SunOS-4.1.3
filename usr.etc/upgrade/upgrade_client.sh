#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# Copyright (c) 1990 Sun Microsystems, Inc.
#
# 3. upgrade_client script
#         a. This script will upgrade the root partitions of some, all, or
#            none of the sun4c clients with either the PIE vmunix_small
#            or a kernel specified by the user.
#         b. This script will also edit the client's fstab to mount the
#            PIE kvm, replace the client's kadb, and revector the client's
#            tftpboot link to boot.sun4c.sunos.4.1_PIE (or whatever its
#            exact name is).
#         c. This script WILL modify /etc/install to reflect the current
#            status of each client.
#

########################################################################
#
# variables
#
AARCH=`/bin/arch`
KARCH=`/bin/arch -k`
INSDIR=/etc/install
RELEASE_STR="${AARCH}.${KARCH}.sunos.${TO_RELEASE}"
KVMPATH=`awk -F= '/kvm_path/ {print $2}' ${INSDIR}/soft_info.${RELEASE_STR}`
DEFAULT_KERNEL=${KVMPATH}/stand/vmunix_small
CANDS=/tmp/clients$$

########################################################################
#
# utility functions
#

#
# find_root client_name
#
find_root() {
	awk -F= '/root_path/ {print $2}' < ${INSDIR}/client.$1
}

#
# fix_kernel kernel_path root_directory
#
fix_kernel() {
	cp $1 $2/vmunix
}

#
# fix_kadb kadb_path root_directory
#
fix_kadb() {
	cp $1 $2/kadb
}

#
# fix_fstab root_directory kvm_path
# Replace the /usr/kvm mountpoint and add the pcfs line if not present.
#
fix_fstab() {
	FSTAB=$1/etc/fstab
	if [ ! -s $FSTAB ]; then
		echo "No $FSTAB?"
		return 1
	fi
	mv $FSTAB ${FSTAB}.orig
	expand $FSTAB.orig | grep -v ' /usr/kvm ' | unexpand -a > $FSTAB
	echo "`hostname`:$KVMPATH	/usr/kvm nfs ro 0 0" >> $FSTAB
	echo "/dev/fd0	/pcfs	pcfs rw,noauto 0 0" >> $FSTAB
}

#
# fix_tftpboot client_name
#
fix_tftpboot() {
	INET=`awk "/$1/ {print \\$1}" /etc/hosts | \
	    awk -F. '{printf("%02x%02x%02x%02x\n", $1, $2, $3, $4);}' | \
	    tr '[a-z]' '[A-Z]'`
	BOOT=`echo boot.$TO_RELEASE`
	rm -f /tftpboot/$INET /tftpboot/$INET.SUN4C
	ln -s $BOOT /tftpboot/$INET
	ln -s $BOOT /tftpboot/$INET.SUN4C
}

#
# fix_installfiles client_name
#
fix_installfiles() {
	echo $1 >> ${INSDIR}/client_list.$RELEASE_STR
	mv ${INSDIR}/client.$1 /tmp/$$
	sed "s@^kvm_path.*@kvm_path=${KVMPATH}@" /tmp/$$ > ${INSDIR}/client.$1
	rm /tmp/$$
}

get_yn() {
	echo -n $*'? [yes] '
	read answer
	case $answer in
		y* | Y*)	return 0;;
		"")		return 0;;
		*)		return 1;;
	esac
}

########################################################################
#
# body of program
#
USAGE="`basename $0` [ -a ] [ -k kernel ] [ client ... ]"
INTERACTIVE=true
ALL=false
KERNEL=""

while getopts ak: i; do
	case $i in
	a)	INTERACTIVE=false
		ALL=true;;
	k)	KERNEL=$OPTARG;;
	\?)	echo $USAGE
		exit 2;;
	esac
done
shift `expr $OPTIND - 1`
if [ $# -gt 0 ]; then
	INTERACTIVE=false
	CLIENTS=$*
fi

#
# Find all the candidates. Take the union of
#	/etc/install/client_list.sun4.sun4c.sunos.4.1
#	/etc/install/client_list.sun4.sun4c.sunos.4.1_PSR_A_*
#
for i in ${FROM_RELEASE_LIST}; do
	client_file="${INSDIR}/client_list.${AARCH}.${KARCH}.sunos.$i"
	if [ -f ${client_file} ]; then
		cat ${client_file}
	fi
done | sort -u > $CANDS

#
# Are there any candidates?
#
if [ `wc -l < $CANDS` -eq 0 ]; then
        echo "No clients to upgrade."
	exit 0
fi

#
# Unless told otherwise, ask which ones we should do.
#
while $INTERACTIVE; do
	CLIENTS=""
	echo "Upgrade the following clients to ${TO_RELEASE}?"
	for i in `cat $CANDS`; do
		if get_yn $i; then
			CLIENTS="$CLIENTS $i"
		fi
	done
	test -z "$CLIENTS" && rm $CANDS && exit 0
	echo "These clients will be upgraded:"
	echo "	" $CLIENTS
	if get_yn Is this acceptable; then
		break
	fi
done
#
# If ALL, do all of them.
#
if $ALL; then
	CLIENTS=`cat $CANDS`
fi
rm $CANDS

#
# How about the kernel?
#
while true; do
	if [ -z "$KERNEL" ]; then
		if $INTERACTIVE; then
			if get_yn Use default GENERIC_SMALL kernel; then
				KERNEL=$DEFAULT_KERNEL
			else
				echo -n 'Path name of kernel to install? '
				read KERNEL
			fi
		else
			KERNEL=$DEFAULT_KERNEL
		fi
	fi
	if [ -f "$KERNEL" ]; then
		break
	fi
	echo "File $KERNEL does not exist."
	if $INTERACTIVE; then
		KERNEL=""
	else
		exit 1
	fi
done

#
# All systems go: we have a list of clients and a valid kernel.
# Do it to it.
#
for i in $CLIENTS; do
	echo Upgrading client $i...
	ROOT=`find_root $i`
	echo '	installing kernel'
	fix_kernel $KERNEL $ROOT
	echo '	installing kadb'
	fix_kadb ${KVMPATH}/stand/kadb $ROOT
	echo '	updating fstab'
	fix_fstab $ROOT $KVMPATH
	echo '	updating tftpboot link'
	fix_tftpboot $i
	echo '	updating installation records'
	fix_installfiles $i
done

exit 0
