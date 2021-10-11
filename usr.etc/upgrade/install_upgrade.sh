#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
#       Copyright (c) 1990, Sun Microsystems, Inc.  All Rights Reserved.
#       Sun considers its source code as an unpublished, proprietary
#       trade secret, and it is available only under strict license
#       provisions.  This copyright notice is placed here only to protect
#       Sun in the event the source is deemed a published work.  Dissassembly,
#       decompilation, or other means of reducing the object code to human
#       readable form is prohibited by the license agreement under which
#       this code is provided to the user or company in possession of this
#       copy.
#
#       RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
#       Government is subject to restrictions as set forth in subparagraph
#       (c)(1)(ii) of the Rights in Technical Data and Computer Software
#       clause at DFARS 52.227-7013 and in similar clauses in the FAR and
#       NASA FAR Supplement.
#
# NAME: install_upgrade
#
# DESCRIPTION:
#       
#       This script provides a wrapper for the entire upgrade function.
#
#       It named appropriately so that it can be automatically
#       invoked from extract_patch.
#
#       It calls the upgrade_exec and upgrade_client scripts.
#
#       You must be root to run this script.
#
# USAGE:
#
#       install_upgrade
#       

PATH=/usr/ucb:/bin:/usr/bin:/etc:/usr/etc
export PATH

CMD=`basename $0`
KARCH=`/bin/arch -k`
INSDIR=/etc/install
host_type=`awk -F= '$1 ~ /^sys_type$/ {print $2}' $INSDIR/sys_info`

# release that upgrade will upgrade to
TO_RELEASE="4.1_PIE_ALPHA1"

# list of releases that upgrade will run on
FROM_RELEASE_LIST="4.1 4.1_PSR_A"

# release currently installed
FROM_RELEASE=`sed "s/^.*sunos\.//" $INSDIR/release`

export TO_RELEASE FROM_RELEASE_LIST FROM_RELEASE

# is the current release one in the list?
release_okay() {
	for i in ${FROM_RELEASE_LIST}; do
       		[ ${FROM_RELEASE} = $i ] && return 0
	done
	return 1   
}

# find if there are any clients of a different karch
find_clients() {
	clients=`/bin/ls ${INSDIR}/client.* 2> /dev/null`
	for i in $clients; do
		grep "arch_str" $i | grep -s "$KARCH" || return 1
	done
	return 0
}

########################################################################
#
# Beginning of main program.
#
########################################################################
 
trap 'echo $CMD ABORTED.; exit 1' 1 2 15

# you must be root

if [ "`whoami`x" != "root"x ]; then
    echo "You must be root to run $CMD."
    exit 1
fi
 
# a check should be put here to make sure any files needed in
# /etc/install are in existence, to prevent errors later on.  



# is the current release one in the list?

release_okay
if [ $? -ne 0 ]; then
	echo "$CMD: Upgrade was designed to upgrade the following releases:"
	echo ${FROM_RELEASE_LIST}
	echo "$FROM_RELEASE release is currently installed on this system."
	exit 1
fi

# check for non-<karch> clients

find_clients
if [ $? -ne 0 ]; then
	echo 
        echo "$CMD: Upgrade was designed to run on a standalone or"
	echo "homogeneous server.  Non-${KARCH} clients found."
	exit 1
fi

# a check should be put in here to make sure there is enough
# free disk space



# start upgrade

echo ""
echo  "***** Start $CMD  `date` *****"
echo ""
echo "$CMD will execute the following scripts to do an upgrade:"
echo ""
echo "    1) \"upgrade_exec\" - reads upgrade files from the"
echo "        release media and installs them into the onto the host." 
echo "    2) \"upgrade_client\" - upgrades client's root and fstab"

echo; echo "starting upgrade_exec..."
./upgrade_exec
[ $? -ne 0 ] && exit 1

echo; echo "starting upgrade_client..."
./upgrade_client
[ $? -ne 0 ] && exit 1

echo ""
echo  "***** End $CMD  `date` *****"

