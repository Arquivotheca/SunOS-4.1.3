#!/bin/sh
#set -x
#
# %Z%%M% %I% %E%
#
# @(#)extract_patch.sh 1.1 90/01/13
#       Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.
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
# NAME: extract_patch 
#
# DESCRIPTION:
# 	Front end to patch installations.  This will look at the
#	table of contents on a release tape and extract the named
#	"patch" from the patch area of the release tape.
#
#
# USAGE:
#	extract_patch [-dDEVICE] [-rREMOTE_HOST] [-DEFAULT] [-ppatch_name]
#       
#    DEVICE = tape device (st?, mt?)
#    REMOTE_HOST = host of remote tape drive
#    DEFAULT = use all default values
#
#
# STRATEGY:
#
#    1. Get the "user input's" that we need to know.  Ask all questions
#	before actually doing anything.  The things we need to know:
#
#	patch name
#	tape drive name
#	tape drive machine (if remote)
#	installation directory
#
#    2. Validate that the tape drive is accessible.
#
#    3. Read TOC to find out what volume the patch is on.  If the patch
#	is not found, give error and die.
#
#    4. Make sure correct tape is mounted.  If not, give message, wait
#	for new tape to be mounted, and repeat.
#
#    5. Read tape volume into PATCH_DIR (default: /usr/tmp/<name of patch>).
#
#    6. Run install_patch program, which will do any patch specific
#	installation required.
#
#
# Change History:
#
# 1.2 by ichiban@munchkin - added CDROM support. 
#
PATH=/usr/ucb:/bin:/usr/bin:/etc:/usr/etc
export PATH

ARCH=`arch -k`
CD="cd"
CDROM=""
CPFILE="/tmp/cp.$$"
DD="dd"
DEFAULT=""
DEVROOT=""
EXPR="/usr/bin/expr"
MORE="more"
MT="mt"
PARAMS=""
README="README"
REMOTE=""
REMOTE_HOST=""
TAR="tar"
XDRTOC="/usr/etc/install/xdrtoc"

PATCH_PREFIX="Patch_"

USAGE="$0 [-dDEVICE] [-rREMOTE_HOST] [-DEFAULT] [-pPATCH_NAME]"

########################################################################
#
#
# utility routines
#
#
########################################################################

#
# the only place we read from stdin
#
stdin_log () {
    read $1
}

#
# return "yes" or "no" into shell variable named $1.  $2 is the user prompt.
#
get_yesno () {
    yesno=""
    while [ ! "$yesno" ]
    do
        echo ""
        echo -n "$2 [yes|no]? "
        stdin_log yesno
        case "$yesno" in
        [yY]*)  yesno=yes ;;
        [nN]*)  yesno=no ;;
        *)      echo "" ; echo 'Please enter "yes" or "no".' ; yesno="" ;;
        esac
    done
    eval $1="$yesno"
}

#
# return "yes" or "no" into shell variable named $1.  $2 is the user prompt.
# $3 is the default answer to use
#
get_yesno_def () {
    yesno=""
    while [ ! "$yesno" ]
    do
        echo ""
        echo -n "$2 [yes|no]($3)? "
        stdin_log yesno
        case "$yesno" in
        [yY]*)  yesno=yes ;;
        [nN]*)  yesno=no ;;
        "")     yesno="$3" ;;
        *)      echo "" ; echo 'Please enter "Yes" or "No".' ; yesno="" ;;
        esac
    done
    eval $1="$yesno"
}
 

#
# give the user a chance to bail out (no arguments)
#
check_continue () {
    if [ "$DEFAULT" ]
    then
	cleanup 1
    else
	get_yesno_def continue "Do you want to continue" yes
	if [ "$continue" = no ]
	then
	    echo ""
	    echo "$0 : Terminating"
	    cleanup 1
	fi
    fi
}


#
# generic user prompt routine with no default
# $1 is the variable to set
# $2 is the prompt
#
get_ans () {
    echo ""
    echo -n "$2? "
    stdin_log new_ans
    eval $1="$new_ans"
}

#
# generic user prompt routine with default
# $1 is the variable to set
# $2 is the prompt
# $3 is the default answer
#
get_ans_def() {
    echo ""
    echo -n "$2 [$3]? "
    stdin_log new_ans
    if [ "$new_ans" = "" ]
    then
	new_ans="$3"
    fi
    eval $1="$new_ans"
}

#
# provide a single script exit point -- clean up and go away
#
cleanup () {
    rm -f "$CPFILE"
    exit $1
}


#
# visually separate unrelated sections
#
newsection() {

    if [ ! "$DEFAULT" ]
    then
	echo ""
	echo "--------------------------"
    fi
}


#
# rewind a tape
#
tape_rewind() {
    if [ "$REMOTE" ]
    then
	stat=""
	stat=`rsh -n "${REMOTE_HOST}" "$MT -f ${DEVPATH} rew; echo \\$status"`
	case $stat in
	[1-9]* | 1[0-9]* )
	    echo
	    echo "$0 : Problem with seeking tape drive, exiting"
	    cleanup 1
	    ;;
	*)
	    ;;
	esac
    else
	$MT -f ${DEVPATH} rew $1
	if [ $? -ne 0 ]
	then
	    echo
	    echo "$0 : Problem with seeking tape drive, exiting"
	    cleanup 1
	fi
    fi
}

#
# forward space the specified number of files
# number of files to forward space is $1
#
tape_fsf() {
    if [ "$REMOTE" ]
    then
	stat=""
	stat=`rsh -n "$REMOTE_HOST" "$MT -f ${DEVPATH} fsf $1; echo \\$status"`
	case $stat in
	[1-9]* | 1[0-9]* )
	    echo
	    echo "$0 : Problem with seeking tape drive, exiting"
	    cleanup 1
	    ;;
	*)
	    ;;
	esac
    else
	$MT -f ${DEVPATH} fsf $1
	if [ $? -ne 0 ]
	then
	    echo
	    echo "$0 : Problem with seeking tape drive, exiting"
	    cleanup 1
	fi
    fi
}

#
# wait for some user input
#
return_when_ready () {
    if [ ! "$DEFAULT" ]
    then
	echo
	echo -n "Press return when ready:"
	read ans
    fi
}


#
# read the table of contents in.
#
extract_toc() {
    if [ "$REMOTE" ]
    then
	command="rsh -n "${REMOTE_HOST}" $DD if=${DEVPATH}"
    else
	command="$DD if=${DEVPATH}" 
    fi

    #
    # sigh.  you must be super user to use xdrtoc...
    #

    #
    # we set six variables:
    #	TAPE_ARCH -- the architecture for this tape
    #	TAPE_VOLUME -- which volume this tape is
    #	PATCH_VOLUME -- volume that the patch is on
    #	PATCH_FILE -- the file on that volume that the patch is stored in
    #	PATCH_SIZE -- the size of that patch tape file
    #   AVAIL_PATCHES -- the list of all patches on the tape
    PATCH_VOLUME=""
    PATCH_FILE=""
    PATCH_SIZE=""

    eval `$command | ${XDRTOC} | awk '
	$1 == "ARCH" { printf( "TAPE_ARCH=%s ", $2 ) }
	$1 == "VOLUME" { printf( "TAPE_VOLUME=%s ", $2 ) }
	$3 == "'${PATCH_PREFIX}${PATCH}'" {
	    printf( "PATCH_VOLUME=%s PATCH_FILE=%s PATCH_SIZE=%s ",$1,$2,$4 );
		}
	$3 ~ /'${PATCH_PREFIX}'.*/ { patches = patches " " $3 }
	END { printf( "AVAIL_PATCHES=\"%s\"\n", patches ) } ' `
	
}


########################################################################
#
#
# Parse the arguments
#
#
########################################################################

for param in $*;
do
    PARAMS="$PARAMS $param"
    case "$param" in
    -d*)
	DEVROOT=`$EXPR $param : '-d\(.*\)' '|' "1"` 
	if [ $DEVROOT -eq 1 ]
	then
	    echo
	    echo "$0 : Invalid device name"
	    echo "            USAGE : $USAGE"
	    cleanup 1
	fi
	CD_DEV=`expr $DEVROOT : '\(sr\)'`
	if [ "$CD_DEV" = "sr" ]; then
	    CDROM=1
	fi
	;;

    -r*)
	REMOTE_HOST=`$EXPR $param : '-r\(.*\)' '|' "1"` 
	if [ $REMOTE_HOST -eq 1 ]
	then
	    echo
	    echo "$0 : Invalid Host name"
	    echo "            USAGE : $USAGE"
	    cleanup 1
	fi
	REMOTE="yes"
	;;

    -p*)
	PATCH=`$EXPR $param : '-p\(.*\)' '|' "1"` 
	if [ $PATCH -eq 1 ]
	then
	    echo
	    echo "$0 : Invalid Patch name"
	    echo "            USAGE : $USAGE"
	    cleanup 1
	fi
	;;

    -DEFAULT)
	DEFAULT=true
	;;

    *)
	echo
	echo USAGE : $USAGE
	cleanup 1
	;;
    esac
done

########################################################################
#
#
# Prompt user for device location
#
#
########################################################################

newsection

#
# get the name of the patch
#

while [ ! "$PATCH" ]
do
    get_ans PATCH "Enter the name of the patch you wish to extract"
done

newsection

#
# ask if using CDROM (if user did not indicate device on command line)
#

if [ ! "$DEVROOT" ]
then
	get_yesno_def ans "Are you using CDROM for this operation" no
	if [ "$ans" = yes ]
		then
		newsection
		get_ans DEVROOT 'Enter CDROM Device Name (e.g. sr0, sr1)'
		CDROM=1
	fi
fi

#
# This section is for CDROM support
#

if [ "$CDROM" -eq 1 ]
then
	mount -rt hsfs /dev/${DEVROOT} /usr/etc/install/tar
	count=`ls -d /usr/etc/install/tar/patches/sunos* | wc -w`
	if [ "$count" -ge 2 ]
	then
		echo "Multiple software releases found on disc:"
		echo ""
		cd /usr/etc/install/tar/patches
		ls -d sunos*
		echo ""
		get_ans RELEASE "Enter the name of the release you want"
		REL_DIR="/usr/etc/install/tar/patches/${RELEASE}"
	else
		REL_DIR=`ls -d /usr/etc/install/tar/patches/sunos*`
	fi
	cd /usr/tmp
	if [ ! -d $PATCH ]
	then
		mkdir $PATCH
	fi
	cd $PATCH
	tar xf ${REL_DIR}/patch_${PATCH}
	status=$?
	if [ "$status" -ne 0 ]
	then
		cd ..
		rm -rf $PATCH
		echo "Could not extract patch from disk."
		echo "Are you sure you have the right patch name?"
		echo "FYI: here's a list of the available patches..."
		cd $REL_DIR
		ls *
		cd /
		umount /usr/etc/install/tar
		exit 1
	fi
	./install_${PATCH}
	status=$?
	if [ "$status" -ne 0 ]
	then
		 echo "Could not install patch."
	fi
	cd ..
	rm -rf $PATCH
	cd /
	umount /usr/etc/install/tar
	exit 0

#
# end of CDROM support section
#

else

newsection

#
# determine local vs. remote tape drive
#
if [ ! "$REMOTE" -a ! "$DEVROOT" ]
then

    tapeloc=""

    while [ ! "$tapeloc" ]
    do
	get_ans_def tapeloc \
	    "Enter media drive location [local | remote]" "local"

	case "$tapeloc" in
	[rR]*)
	    get_ans REMOTE_HOST "Enter hostname of remote drive"

	    PARAMS="$PARAMS -r$REMOTE_HOST"

	    rsh -n "$REMOTE_HOST" "echo 0 > /dev/null"
	    if [ "$?" -ne 0 ]
	    then
		echo ""
		echo "$0 : Problem with reaching remote host $REMOTE_HOST"
		echo "Please enter another host name."
		tapeloc=""
	    else
		REMOTE="yes"
	    fi
	    ;;
	[lL]*)
	    REMOTE=""
	    ;;
	*)
	    echo ""
	    echo "Please enter either 'local' or 'remote'"
	    tapeloc=""
	    ;;
	esac
    done
fi

echo ""
if [ "$REMOTE" ]
then
    echo "Using a remote tape drive..."
else
    echo "Using a local tape drive..."
fi

newsection

# get the device name

#
# we specifically *don't* check the validity of command line args.
# this is a feature to new, unknown devices.
#
while [ ! "$DEVROOT" ]
do
    get_ans DEVROOT 'Enter Device Name (e.g. st0, mt0)'

    case "$DEVROOT" in
    st*)
	OPTIONS="xvfbp"
	BS=126
	;;
    mt*)
	OPTIONS="xvfbp"
	BS=20
	;;
#   ar*)
#	OPTIONS="xvfbp"
#	BS=126
#	;;
    *)
	echo "$0 : Invalid device name $DEVROOT"
	DEVROOT=""
	;;
    esac

    if [ "$DEVROOT" ]
    then
	PARAMS="$PARAMS -d$DEVROOT"
    fi
done

DEVPATH="/dev/nr${DEVROOT}"

echo ""
echo "Using tape device $DEVPATH..."

newsection

if [ ! "$DEFAULT" ]
then
    echo ""
    echo "**Please mount the release media if you haven't done so already.**"
    # Wait for tape to mount, just in case.

    return_when_ready
fi


########################################################################
#
#
# check the table of contents on the patch tape
#
#
########################################################################

newsection

tape_ok=""

# beginning of loop
while [ ! "$tape_ok" ]
do

    echo ""
    echo "Examining the tape's table of contents..."


    # rewind the tape
    tape_rewind

    # seek to tape file 1
    tape_fsf 1

    # extract the TOC
    extract_toc

    # check for the presence of the patch
    if [ ! "$PATCH_VOLUME" ]
    then
	# if patch not found, display list and exit

	if [ "$AVAIL_PATCHES" ]
	then
	    echo "Could not find patch $PATCH.  Here are the available patches"
	    echo "for this release tape:"
	    echo ""

	    for i in $AVAIL_PATCHES
	    do
		echo "    $i" | sed "s/${PATCH_PREFIX}//"
	    done
	else
	    echo "There are no patches on this release tape."
	fi

	echo ""

	cleanup 1
    fi

    # make sure correct volume is mounted
    if [ "$TAPE_VOLUME" != "$PATCH_VOLUME" ]
    then
	echo ""

	if [ "$DEFAULT" ]
	then
	    echo "You must mount tape volume $PATCH_VOLUME"
	    echo ""
	    cleanup 1
	fi

	echo "You currently have tape volume $TAPE_VOLUME mounted."
	echo "Please mount tape volume $PATCH_VOLUME instead."

	return_when_ready
    else
	# we passed all the tests
	tape_ok=yes
    fi

done

echo ""
echo "Patch ${PATCH} found on tape volume $PATCH_VOLUME file $PATCH_FILE..."

newsection

#
# pick an installation directory
#
# enhancement: check for enough disk space
#
while [ ! "$PATCH_DIR" ]
do
    get_ans_def PATCH_DIR "Where would you like the patch read into" \
	"/usr/tmp/${PATCH_PREFIX}${PATCH}"

    if [ ! -d "$PATCH_DIR" ]
    then

	get_yesno_def ans "${PATCH_DIR} does not exist.  Create it" yes

	if [ "$ans" = yes ]
	then
	    echo ""
	    echo "Creating the patch directory ${PATCH_DIR}..."

	    mkdir -p "${PATCH_DIR}"

	    if [ ! -d "$PATCH_DIR" ]
	    then
		echo
		echo "Could not create ${PATCH_DIR}"

		check_continue

		# let him pick another name
		PATCH_DIR=""
	    fi

	else
		# let him pick another name
		PATCH_DIR=""
	fi
    fi
done


# change directory to $PATCH_DIR
${CD} "$PATCH_DIR"

echo ""
echo 'Positioning the tape (this may take a while)...'

# rewind the tape
tape_rewind

# seek to the right place on the tape
tape_fsf "$PATCH_FILE"

echo ""
echo "Extracting the patch..."
echo ""

# read in the tape
if [ "$REMOTE" ]
then
	rsh -n  "$REMOTE_HOST" $DD "if=$DEVPATH" bs=${BS}b | $TAR xBvfb - ${BS}
	if [ "$?" -ne 0 ]
	then
		echo
	     	echo "$0 : Patch extraction failed!"
		echo
	     	cleanup 1
	fi
else
	$TAR $OPTIONS $DEVPATH $BS
	if [ "$?" -ne 0 ]
	then
		echo
	     	echo "$0 : Patch extraction failed!"
		echo
		cleanup 1
	fi
fi

# rewind the tape again
tape_rewind

newsection

########################################################################
#
# if there is a readme file, display it
#
########################################################################

if [ -r "${README}" ]
then
    
    get_yesno_def ans \
	"Would you like to see the $README file for this patch" yes

    if [ "$ans" = yes ]
    then

	${MORE} ${README}

    fi
    
fi



########################################################################
#
# run install_patch if it exists
#
########################################################################

install_patch="install_${PATCH}"

if [ -x "$install_patch" ]
then
    get_yesno_def ans "Do you want to run the patch installation program" yes


    if [ "$ans" = yes ]
    then
	echo
	echo "running the $install_patch script"
	echo
	"./$install_patch" $PARAMS
    fi
fi

exit 0
fi
