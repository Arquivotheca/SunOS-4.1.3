#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
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
# NAME: upgrade_exec
#
# DESCRIPTION:
#       
#       Script to selectively replace common (shared) executables
#       with the corresponding 4.1_PIE version.  
#
#       It will modify the appropriate files in /etc/install.  The
#       installed software will be reported as 4.1_PIE.
#
#       It installs from release media tape and cdrom.
#
# USAGE:
#       upgrade_exec [-dDEVICE] [-rREMOTE_HOST]
#       
#       DEVICE = tape device (st?, mt?, sr?)
#       REMOTE_HOST = host of remote tape drive
#
#
# STEPS:
#
#    1. Get the "user input's" that we need to know.  Ask all questions
#	before actually doing anything.  The things we need to know:
#
#	drive name
#	drive machine (if remote)
#
#    2. Validate that the drive is accessible.
#
#    3. Ask user to insert the first volume of the media set.
#       Read in the toc onto disk.
#       Verify volume, arch and release of media.
#
#    4. Get a list of packages to upgrade.  Read through each
#       package listed in the toc, and if the include file list
#       exists and the package is in the media list file, then
#       it should be upgraded.  The order of the list should 
#       be the order the packages are listed the toc.
#       
#    5. Start the installation.
#       Loop through the list of packages until done.
#
#          Read TOC on disk to find out what volume the package is on.  
#          If correct volume isn't mounted, give message to mount
#          the volume, wait for next volume to be mounted, and repeat.  
#
#          If a new volume had to be mounted, read in the toc from
#          tape to disk, get volume number, and verify that it's the correct
#          volume number for this package.  Also verify arch and release.
#
#          Skip tape to correct file.
#
#          Read tarfile into correct location. Use include
#          list to only extract off files that have changed. 
#
#    6. Fix any suninstall files to reflect new release  
#
#    7. Fix the links in /export/exec.
#
#    8. Install vmunix, kadb, bootblock
#
#

INSDIR="/etc/install"
HERE=`pwd`
CMD=`basename $0`
XDRTOC="/usr/etc/install/xdrtoc"
TOCFILE="/tmp/toc.$$"
SUFFIX="include"
AARCH=`/bin/arch`
KARCH=`/bin/arch -k`
CD="cd"
DD="/bin/dd"
EXPR="/usr/bin/expr"
MT="mt"
TAR="tar"
HOST=`hostname`
MEDIA_FILE="${INSDIR}/appl_media_file.${AARCH}.sunos.${FROM_RELEASE}"
REQUIRED="root usr Kvm Install Networking"

USAGE="$CMD [-dDEVICE] [-rREMOTE_HOST]"

DEVROOT=""
DEVPATH="" 
PARAMS="" 
REMOTE="" 
REMOTE_HOST=""
MEDIA="tape"

# general purpose utility subroutines
. ./utility_subs

# toc processing subroutines
. ./toc_subs

# install subroutines
. ./install_subs

########################################################################
#
# Beginning of main program.
#
########################################################################

trap 'cleanup 1' 1 2 15

# Check parameters.
# Set the following variables:
# DEVROOT, DEVPATH, REMOTE, REMOTE_HOST, MEDIA
. ./get_params

# execute media subroutines
case "${MEDIA}" in
    "tape")   . ./tape_subs  ;;
    "cdrom")  . ./cdrom_subs ;;
    *)        echo "$CMD: Invalid device name $MEDIA"
              cleanup 1 ;; 
esac

# mount release media 
echo ""
echo "**Please mount volume 1 of the release media if you haven't 
done so already.**"

# Wait for return
return_when_ready

# For cdrom, mount hsfs
media_start

# extract the TOC
extract_toc

# check volume and make sure volume 1 is mounted
media_check 1

# get list of packages to upgrade in variable UPGRADE_LIST
get_package_list 

if [ "$UPGRADE_LIST" ]; then
    echo 
    echo "These packages will get upgraded: "
    echo "   $UPGRADE_LIST" 
    echo ""
    echo "Ready to start installation."
    return_when_ready	
else
    echo "$CMD: No packages to upgrade, exiting."
    cleanup 1   
fi


# loop through list of packages to upgrade
for PACKAGE in $UPGRADE_LIST 
do
    # sets variables U_VOLUME, U_FILE, U_TYPE
    toc_tar_info 

    # make sure correct volume mounted
    media_check $U_VOLUME

    if [ $U_TYPE = "tarZ" ]; then
        uncompress="/usr/ucb/uncompress"
    else
        uncompress="/bin/cat"
    fi

    echo
    echo "Extracting upgrade for $PACKAGE..."

    # this should never happen, but putting a check in anyway
    list="${HERE}/${PACKAGE}.${SUFFIX}"
    [ ! -s $list ] && echo "Unable to locate ${list}...continuing" && continue
	
    case "${PACKAGE}" in
        "root") $CD / ;;
        "Kvm")  $CD /usr/kvm ;;
        *)      $CD /usr ;;
    esac

    media_extract

done

# rewind the tape again
media_rewind

# fix up the suninstall data files to reflect the new release 
fix_data_files

# fix the release name file 
fix_release

# reset the links in /export/exec
make_links

# install vmunix, kadb, and boot block
FROM=/usr/kvm/stand
install_vmunix $FROM
install_kadb $FROM
install_boot_block 
 
# add pcfs line to fstab
fix_fstab

cleanup 0
