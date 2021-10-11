#!/bin/sh
#set -x
#
# %W%  %G%
#       Copyright (c) 1988, Sun Microsystems, Inc.  All Rights Reserved.
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
# NAME: extract_unbundled 
#
# DESCRIPTION:
# 	Front end to unbundled installations.  This will skip the copyright on
#	an unbundled release tape and execute the installation script which
#	follows.
#	Update: 10/10/88  added extentions for floppy media.  --SMAR
#	        Expects install scripts and copyright.d in a subdir
#               called "+install".
#
# USAGE:
#	extract_unbundled [-dDEVICE] | [-rREMOTE_HOST] | [-DEFAULT]
#       
#    DEVICE = tape device (st0,mt0,ar0)
#    REMOTE_HOST = host of remote tape drive
#    DEFAULT = use all default values
#
# DATE: 01/21/87
PATH=/usr/ucb:/bin:/usr/bin:/etc:/usr/etc
export PATH
WORK_PATH=/usr/tmp/unbundled
INSTALL_SCRIPT=install_unbundled
PARAMS=""
CD="cd"
DD="dd"
MT="mt"
TAR="tar"
BAR="bar"
MV="mv -f"
TAPE_LOC=""
REMOTE_HOST=""
REMOTE=0
DEFAULT=0
DEV_SPEC=0
ANS=""
DISK=0

USAGE="$0 [-dDEVICE] | [-rREMOTE_HOST] | [-DEFAULT]"

# Parse the arguments
for param in $*;
do
	PARAMS="$PARAMS $param"
        case "$param" in
        -d*)
	    DEVROOT=`expr $param : '-d\(.*\)' '|' "1"` 
	    if [ $DEVROOT -eq 1 ]
	    then
		echo
		echo "$0 : Invalid device name"
            	echo "            USAGE : $USAGE"
		exit 1
	    fi
	    DEV_SPEC=1 ;;
        -r*)
	    REMOTE_HOST=`expr $param : '-r\(.*\)' '|' "1"` 
	    if [ $REMOTE_HOST -eq 1 ]
	    then
		echo
		echo "$0 : Invalid Host name"
            	echo "            USAGE : $USAGE"
		exit 1
	    fi
            REMOTE=1;;
        -DEFAULT)
	    ANS="y"
            DEFAULT=1;;
        *)
	    echo
            echo USAGE : $USAGE
	    exit 1;;
        esac
done
# Prompt user for device location

while [ "$TAPE_LOC" =  "" ]
do
    if [ $REMOTE -eq 0 ]
    then
	if [ $DEFAULT -eq 0 ]
	then
		echo
		echo -n "Enter media drive location [local | remote]: "
		read TAPE_LOC
	else
		TAPE_LOC="local"
	fi
    else
	TAPE_LOC="remote"
    fi
    case "$TAPE_LOC" in 
	r*) if [ "$REMOTE_HOST" = "" ]
		then
			echo -n "Enter hostname of remote drive: "
			read REMOTE_HOST
			echo ""
			REMOTE=1
		fi
		PARAMS="$PARAMS -r$REMOTE_HOST"
	  	rsh -n  $REMOTE_HOST "echo 0 > /dev/null"
		if [ "$?" -ne 0 ]
		then
 			echo "$0 : Problem with reaching remote host $REMOTE_HOST"
 			exit 1
		fi;;
	l*) ;;
	*) 
 	   echo "$0 : Incorrect response, use \"local\" or \"remote\""
	   TAPE_LOC="" ;;
    esac
done

# prompt user for device if it hasn't been specified in the command line.

if [ $DEV_SPEC -eq 0 ]
then
#   This one will loop until a correct device is chosen.
    DEVROOT=""
    while [ "$DEVROOT" = "" ] 
    do
   	echo ""
	echo -n "Enter Device Name (e.g. rst0, rmt0, rfd0c) : /dev/r"
	read DEVROOT
        case  "$DEVROOT" in
            st*) OPTIONS="xvfbp"
                  BS=126;;
            mt*) OPTIONS="xvfbp"
                  BS=20;;
            fd[0-9][ac] | fd[0-9])
		  if [ $REMOTE -eq 1 ]
		  then
		      echo "$0 : Remote option cannot be used with diskette installation"
		      exit 1
		  fi
 	  	  OPTIONS="xvfbpZT"
                  BS="18"
		  DISK=1;;
            ar*) OPTIONS="xvfbp"
                  BS=126;;
            *)    echo "$0 : Invalid device name $DEVROOT"
                  DEVROOT="";;
        esac
    done
else 
#   This one will exit if an incorrect device is chosen. (default exec)
    case  "$DEVROOT" in
        st*) OPTIONS="xvfbp"
              BS=126;;
        mt*) OPTIONS="xvfbp"
              BS=20;;
	fd[0-9][ac] | fd[0-9])
	      if [ $REMOTE -eq 1 ]
	      then
		  echo "$0 : Remote option cannot be used with diskette installation"
	          exit 1
	      fi
	      OPTIONS="xvfbpT"
              BS="18"
	      DISK=1;;
        ar*) OPTIONS="xvfbp"
              BS=126;;
        *)    echo "$0 : Invalid device name $DEVROOT"
              exit;;
    esac
fi

if [ $DISK -eq 0 ]
then
        DEVPATH="/dev/nr${DEVROOT}"
	echo
	echo "**Please mount the release media if you haven't done so already.**"
else
        DEVPATH="/dev/r${DEVROOT}"
	echo
	echo "**Please insert the first diskette if you haven't done so already.**"
fi
PARAMS="$PARAMS -d$DEVROOT"

# Wait for tape to mount, just in case.
if [ $DEFAULT -eq 0 ]
then
	echo
	echo -n "Press return when ready:"
	read ans
fi
sleep 8
# For tape devices only since tape can only use remote
# Rewind tape, just to be sure
if [ $REMOTE -eq 1 ]
then
    stat=""
    stat=`rsh -n ${REMOTE_HOST} "$MT -f ${DEVPATH} rew; echo \\$status"`
    case $stat in
	[1-9]* | 1[0-9]* )
	     echo
    	     echo "$0 : Problem with accessing tape drive, exiting"
             exit 1;;
	*)   ;;
    esac
else
    if [ $DISK -eq 0 ]
    then
    	$MT -f ${DEVPATH} rew
    	if [ $? -ne 0 ]
    	then
		echo
    		echo "$0 : Problem with accessing tape drive, exiting"
       		exit 1
	fi
    fi
fi

# Print copyright to the screen for further verification
# Remote install sometimes inter-mixes the stderr and stdout
clear
echo "The following product will be installed:"
if [ $REMOTE -eq 1 ]
then
	rsh -n ${REMOTE_HOST} "$DD if=${DEVPATH}" > /tmp/cp.tmp
	if [ "$?" -ne 0 ]
	then
		echo
		echo $0 : Problem with reading copyright file on release tape. 
		exit 1
	fi
	cat /tmp/cp.tmp
else
	if [ $DISK -eq 0 ]
	then
	    $DD if=${DEVPATH} > /tmp/cp.tmp
	    if [ "$?" -ne 0 ]
	    then
		echo $0 : Problem with reading copyright file on release tape.
		exit 1
	    fi
            cat /tmp/cp.tmp
	else
 	    while true
	    do
	        $BAR ${OPTIONS} ${DEVPATH} ${BS} +install/copyright.d >/dev/null
 	        if [ -f +install/copyright.d ]
		then
			break
		fi
		eject
                echo $0 : Problem with reading copyright file from diskette.
		echo -n 'Please ensure the correct diskette is inserted. Try again [y|n]? '
		while true
		do
			read ANS
			case $ANS in
			    y* )  break;;
			    n* )  echo $0 : Terminating installation...
				  exit 1;;
			    * )
				  echo
				  echo "Enter \"y\" or \"n\""
			esac
		done
            done
	    cat +install/copyright.d
	    \cp +install/copyright.d /tmp/cp.tmp
	    \rm -rf +install
	fi
fi
ANS=""
while [ "$ANS" = "" ]
do
	echo
	echo -n "Do you want to continue [y|n]? "
	read ANS
        case $ANS in
            y* )  continue;;
            n* )  echo $0 : Terminating installation...
                  exit 0;;
            * )
                  echo
                  echo "Enter \"y\" or \"n\""
                  ANS="";;
        esac
done
# tar off install script to /usr/tmp/unbundled and execute it

if [ ! -d $WORK_PATH ]
then
	mkdir $WORK_PATH
	chmod 777 $WORK_PATH
fi
echo $0 : "Extracting Installation Scripts"
stat=""
if [ $REMOTE -eq 1 ]
then
	$CD $WORK_PATH
	stat=""
	rsh -n  $REMOTE_HOST $MT -f $DEVPATH rew
	rsh -n  $REMOTE_HOST $MT -f $DEVPATH fsf 1
	rsh -n  $REMOTE_HOST $DD if=$DEVPATH bs=${BS}b | $TAR xBvfb - ${BS}
	if [ "$?" -ne 0 ]
	then
	     	echo $0 : This tape is missing installation script.
	     	exit 1
	fi
	rsh -n  $REMOTE_HOST $MT -f $DEVPATH rew
else
	$CD $WORK_PATH
	if [ $DISK -eq 0 ]
	then
		$MT -f $DEVPATH rew
		$MT -f $DEVPATH fsf 1
		$TAR $OPTIONS $DEVPATH $BS
		if [ "$?" -ne 0 ]
		then
			echo
			echo $0 : This tape is missing an installation script.
			exit 1
		fi
		$MT -f $DEVPATH rew
	else
 	    while true
	    do
		$BAR ${OPTIONS} ${DEVPATH} ${BS} +install
 	        if [ -d +install ]
		then
			break
		fi
		eject
		echo $0 : This diskette is missing an installation script.
		echo -n 'Please ensure the correct diskette is inserted. Try again [y|n]? '
		while true
		do
			read ANS
			case $ANS in
			    y* )  break;;
			    n* )  echo $0 : Terminating installation...
				  exit 1;;
			    * )
				  echo
				  echo "Enter \"y\" or \"n\""
			esac
		done
            done
	    ${MV} +install/* ${WORK_PATH}
	    rmdir $WORK_PATH/+install
	fi
fi
echo
echo "$0 : Begin Install Script Execution"
# Note: the -f tells the install script that is was called 
#       from extract_unbundled
$WORK_PATH/$INSTALL_SCRIPT $PARAMS -f
