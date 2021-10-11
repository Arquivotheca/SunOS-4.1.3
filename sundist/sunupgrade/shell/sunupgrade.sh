#!/bin/sh
#
#       @(#)sunupgrade.sh	1.6 8/28/91 SMI
#
##################################################
get_param(){
# gets parameters from the user
# resets the following variables:
# DUMMY, VOLATILE_LIST, VMUNIX, QUIET, 
# VERBOSE, INTERACTIVE
# by parameters provided by the user

for param in $*; do
    case "$param" in

    -d | -dummy)
        DUMMY=true 
 	;;

    -v | -verbose)
        VERBOSE=true ;;

    -q | -quiet)
        QUIET=true ;;

    -n | -noninteractive)
        INTERACTIVE=false 
 	echo "Performing the noninteractive upgrade";;

    -x*)
        VOLATILE_LIST=`expr $param : '-x\(.*\)'`
        if [ x_$VOLATILE_LIST = x_ ] || \
	[ ! -s $VOLATILE_LIST ]; then
            echo $CMD: \
		"Cannot find VOLATILE_LIST " $VOLATILE_LIST
            echo USAGE : $USAGE
	    pull_out 1
        fi
	DO_VOLATILE_TEST=true;;

    -g | -debug)
	set -x
	DEBUG=-x;;

    -w*)
        OPENWINHOME=`expr $param : '-w\(.*\)'`
        if [ x_$OPENWINHOME = x_ ] || \
	[ ! -d $OPENWINHOME ]; then
            echo $CMD: \
		"Cannot find OPENWINHOME " $OPENWINHOME
            echo USAGE : $USAGE
	    pull_out 2
        fi
        ;;

    -nb | -nobackup)
	BACKUP=false;;

    -s | -small)
        NEW_KERNEL=vmunix_small;;
 
    *)
        echo Cannot interpret command line parameter $param.
        echo "USAGE : $USAGE"
        pull_out 3 ;;
    esac
done
}
##################################################
pull_out(){
    echo Exit code $1
    exit
}
##################################################
#
#
COMMON_PATH="/bin:/usr/bin:/usr/ucb:/etc:/usr/etc"

# Before anything is done, make sure a good path is known.
PATH=$COMMON_PATH:$PATH

CMD=`basename $0`
USAGE="$CMD [-dummy] [-verbose] [-quiet] [-noninteractive] [-nobackup] [-small] [-xVOLATILE_LIST] [-wOPENWINHOME]"

TMP=/tmp/$$
USR_TMP=/usr/tmp/sunupgrade
UPGRADE=/usr/etc/install/tar/sunupgrade
LOG=/usr/tmp/$CMD.log
COALESCE_LIST=/usr/tmp/coalesce_list
DUMMY=false
VERBOSE=false
QUIET=false
INTERACTIVE=true
BACKUP=true
DO_VOLATILE_TEST=false
OPENWINHOME=/usr/openwin
NEW_KERNEL=vmunix
VOLATILE_LIST=$UPGRADE/volatile_list
COM_PLACE=$UPGRADE/toolkit
# Remind: This is likely to change as files are added/removed
CRIT_FILES="check_install check_test exec_upgrade formatter house_keeper inout script_gen upgrade upgrade_client"

#
# We are assumed to be running out of the shell directory.  Make sure that
# all of the necessary files are present, otherwise exit with an error.
# If all the files are indeed present, proceed.
#
for FILES in $CRIT_FILES; do
	if [ ! -f $FILES ]; then
		echo "Couldn't find file \"$FILES\" in current working directory."
		echo "Please make sure you are running $CMD from the shell directory."
		pull_out 5
	fi
done

rm -f $LOG

get_param $*

if $DUMMY ; then
    :
else
#
# Look who is running
#
    if [ "`whoami`x" != "root"x ]; then
	echo "You must be root to do $CMD !"
        echo "You may run only \"$CMD -dummy\" as a regular user"
	pull_out 4
    fi
#
    LOG=/etc/install/$CMD.log
fi
if [ ! -d $USR_TMP ]; then
    mkdir $USR_TMP
fi

export CMD USR_TMP TMP UPGRADE LOG COALESCE_LIST DUMMY VERBOSE QUIET INTERACTIVE BACKUP DO_VOLATILE_TEST OPENWINHOME NEW_KERNEL VOLATILE_LIST COM_PLACE PATH

echo $CMD: "effective options list:" >$LOG
 
echo $* >>$LOG

. ./house_keeper
. ./inout

trap 'cleanup 71 ' 1
trap 'cleanup 72 ' 2
trap 'cleanup 73 ' 3
trap 'cleanup 70 ' 10
trap 'cleanup 75 ' 15

$COM_PLACE/sh $DEBUG ./upgrade 2>&1 | $COM_PLACE/tee -a $LOG

