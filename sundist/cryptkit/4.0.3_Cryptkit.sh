:! /bin/sh
: set -x
:
: @(#)4.0.3_Cryptkit.sh 1.1 92/07/30
: from 2.0_Template 2.5 10/20/88
:	Copyright (c) 1988, Sun Microsystems, Inc.  All Rights Reserved.
:	Sun considers its source code as an unpublished, proprietary
:	trade secret, and it is available only under strict license
:	provisions.  This copyright notice is placed here only to protect
:	Sun in the event the source is deemed a published work.  Dissassembly,
:	decompilation, or other means of reducing the object code to human
:	readable form is prohibited by the license agreement under which
:	this code is provided to the user or company in possession of this
:	copy.
:
:	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
:	Government is subject to restrictions as set forth in subparagraph 
:	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
:	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
:	NASA FAR Supplement.
:
: Name: 4.0.3_Cryptkit.sh
:
: Description: 
: It is called from the script "install_unbundled".
: This is the unbundled installation script template.  It contains features
: that were compiled from various groups requests.
: The variables have been separated into the file install_parameters.
: Read the comments in install_parameters to determine how to customize
: the scripts for your product.
:
: NOTE: This is not a generic script! Changes specific to cryptkit
: installation are bracketed by "XXX CRYPTKIT" and must be preserved
: if this script is updated.
: 
: USAGE:
:       4.0.3_Cryptkit.sh [-dDEVICE]  [-rREMOTE_HOST]  [-DEFAULT]
:
:    DEVICE = media device
:    REMOTE_HOST = host of remote tape drive
:    DEFAULT = use all default values
:
:####################################################
:####  Static Variables (Do not change these) #######
:####################################################
: Install directory and Log file for installation
PATH=/usr/ucb:/bin:/usr/bin:/etc:/usr/etc
export PATH
PARAM_FILE="install_parameters"
SH_NAME=`basename ${0}`
LOGFILE="${install_dir}/$SH_NAME.log"
: This is the online software list, updated after each successfull installation
UNBUNDLED_INVENTORY="${install_dir}/Unbundled_Inventory"

:###############################################################
: RUN_TIME VARIABLES (determined by environment or user input) #
:###############################################################
: Operating System
SOS_LEVEL=`strings /vmunix \
        | egrep '^Sun UNIX|^SunOS (Release)' \
        | sed -e 's/.*Release \([^ ][^ ]*\) .*/\1/'`
: Just the OS release number, with decimal point, but w/o ALPHA/BETA/whatever.
SOS_RELEASE=`echo $SOS_LEVEL | sed -e 's/[a-zA-Z].*//'`
: Strip the decimal out of release number
SOS_NUMERIC=`echo $SOS_LEVEL | sed -e 's/\.//'` 
: Strip out any alpha characters (e.g. 40BETA)
SOS_NUMERIC=`expr substr $SOS_NUMERIC 1 2` 
: login of installer
INSTALLER=`whoami`

: host machine architecture
ARCH=`arch`

: available disk space
DISK_AVAIL=`df $DESTDIR | awk '/\$DESTDIR/ {print $4 }'`

: standalone installation
STANDALONE="N"

: server installation
SERVER=""
:########################
: Other Misc. variables #
:########################
CD="cd"
DD="dd"
MT="mt"
TAR="tar"
BAR="bar"

TAPE_LOC=""
REMOTE_HOST=""
REMOTE=0
DEFAULT=0
DEV_SPEC=0
Pre=0
USAGE="$SH_NAME [-dDEVICE] [-rREMOTE_HOST] [-DEFAULT]"
ANS=""
DISK=0
:#############################
: Function for logging Stdin #
:#############################
stdin_log () {

	read $1
	eval echo "\$$1" >> $LOGFILE
}
:###################################
: Function for Disk Space checking #
:###################################
: DESTDIR and partition is actually interchangeable since the "df" will
: pull out the partition part for space checking.  If the user opts to
: change partitions due to lack of space, a directory can be entered
: as a partition and the partition can be can be "space checked".
space_test () {
DESTDIR=$1
tmp=`df $DESTDIR 2>/dev/null | egrep /dev`
if [ "$?" -ne 0 ]
then
	echo
        echo "$SH_NAME : $DESTDIR not available for installation"
        exit 1
fi
partition=`df $DESTDIR | egrep /dev | awk '{print $6}'`
DISK_AVAIL=`df $partition | egrep $partition | awk '{print $4 }'`
:## This is special disk space checking for Sun386i ####
:DISK_AVAIL=`df  /files | egrep /dev | awk '{print $4 }'`
if [ "$?" -ne 0 ]
then
	echo
        echo "$SH_NAME : Partition $partition not available"
        exit 1
fi
while [ $DISK_AVAIL -lt $SOFT_SIZE ]
do
    echo
    echo "Insufficient space available in partition $partition"
    if [ $DEST_SETABLE = "Y" ]
    then
        answer=""
        while [ "$answer" = "" ]
        do
	    	echo
                echo -n "Do you want to change to a different partition [y|n]? "
                stdin_log answer
                echo ""
	    case "$answer" in
                y*|Y*)
		    echo
	            echo "Here is a list of available partitions and free disk space:"
	            df | egrep /dev | awk '{print "    " $6 "      " $4}'
                    partition=""
                    while [ "$partition" = "" ]
                    do
			 echo
                         echo -n "Enter new partition: "
                         stdin_log DESTDIR
		         tmp=`df $DESTDIR 2>/dev/null | egrep /dev`
                         if [ $? != 0 ]
                         then
			     echo
                             echo "$SH_NAME : $DESTDIR not available for installation"
                             partition=""
                         else
		             partition=`df $DESTDIR | egrep /dev | awk '{print $6}'`
                             DISK_AVAIL=`df $partition | egrep $partition | awk '{print $4 }'`           
                         fi
                     done;;
                n*|N*)
                     echo "$SH_NAME : Terminating..."
                     exit 0;;
                *)   
		     echo 
		     echo "Enter \"y\" or \"n\""
		     answer="";;
	    esac
	done
     else
          echo "$SH_NAME : Terminating..."
          exit 0
     fi
done
}

:##################  Begin Installation Script ##############################
echo "$SH_NAME : Begin Installation of $PROD" >> $LOGFILE

: work-around to free up the /tmp space used when uncompressing files with
: the bar command
\rm -f /tmp/tmp_*.Z

:##################################################
:  Added extract_unbundled code here so it works  # 
:  with or without the front-end script.          #
:##################################################
: Parse the arguments
if [ $# -gt 0 ]
then
    for param in $*;
    do
	PARAMS="$PARAMS $param"
        case "$param" in
        -d*)
	    DEVROOT=`expr $param : '-d\(.*\)' '|' "1"` 
	    if [ $DEVROOT -eq 1 ]
	    then
		echo "$SH_NAME : Invalid device name $param"
            	echo "            USAGE : $USAGE"
		exit 1
	    fi
	    DEV_SPEC=1 ;;
        -r*)
	    REMOTE_HOST=`expr $param : '-r\(.*\)' '|' "1"` 
	    if [ $REMOTE_HOST -eq 1 ]
	    then
		echo "$SH_NAME : Invalid Host name $param"
            	echo "            USAGE : $USAGE"
		exit 1
	    fi
            REMOTE=1 ;;
        -DEFAULT)
	    STANDALONE="Y"
	    DEST_SETABLE="N"
	    ANS="y"
            DEFAULT=1 ;;
        -f) Pre=1 ;;
        *)  echo USAGE : $USAGE
	    exit 1;;
        esac
    done
fi
: Prompt user for device name

while [ "$TAPE_LOC" =  "" ]
do
    if [ $REMOTE -eq 0 ]
    then
	if [ $DEFAULT -eq 0 -a $Pre -eq 0 ]
	then
	    while [ "$TAPE_LOC" = "" ]
	    do
		echo
		echo -n "Enter media drive location (local | remote): "
		stdin_log TAPE_LOC
	    done
	else
		TAPE_LOC="local"
	fi
    else
	TAPE_LOC="remote"
    fi
        case "$TAPE_LOC" in 
	r*)     while [ "$REMOTE_HOST" = "" ]
		do
			echo 
			echo -n "Enter hostname of remote drive: "
			stdin_log REMOTE_HOST
		done
		REMOTE=1
:               Test to see if remote host responds
		rsh  -n $REMOTE_HOST "echo 0 > /dev/null"
		if [ "$?" -ne 0 ]
		then
			echo "$SH_NAME : Problem with reaching remote host $REMOTE_HOST"
			exit 1
		fi;;
	l*) ;;
	*)   echo
	     echo "$SH_NAME : Incorrect response, use \"local\" or \"remote\""
	     TAPE_LOC="" ;;
	esac
done

:  if the device was not specified in the argument line or as "default".
if [ $DEV_SPEC -eq 0 ]
then
:   This one will loop until a correct device is chosen.
: XXX CRYPTKIT begin
: Insist that a unit number be specified along with the device.
    while [ "$DEVROOT" = "" ] 
    do
	echo ""
	echo -n "Enter Device Name (e.g. rst8, rmt8, rfd0c) : /dev/r"
	stdin_log DEVROOT
	case  "$DEVROOT" in
	    st[0-9]*) OPTIONS="xfbp"
		  BS=126;;
	    mt[0-9]*) OPTIONS="xfbp"
		  BS=20;;
	    fd[0-9]*) if [ $REMOTE -eq 1 ]
                  then
                       echo "$SH_NAME : Remote option cannot be used with diskette installation"
                       exit 1
                  fi
		  OPTIONS="xfbpTZ"
		  BS="18"
		  DISK=1;;
	    ar[0-9]*) OPTIONS="xfbp"
		  BS=126;;
	    *)    echo "$SH_NAME : Invalid device name $DEVROOT"
		  DEVROOT="";;
	esac
    done
else
:   This one will exit if an incorrect device is chosen.
    case  "$DEVROOT" in 
        st[0-9]*)   OPTIONS="xfbp"
    	      	BS=126;;
        mt[0-9]*)   OPTIONS="xfbp"
	      	BS=20;;
        fd[0-9]*)   if [ $REMOTE -eq 1 ]
                then
                        echo "$SH_NAME : Remote option cannot be used with diskette installation"
                        exit 1
                fi
		OPTIONS="xfbpTZ"
	      	BS="18"
		DISK=1;;
        ar[0-9]*)   OPTIONS="xfbp"
	      	BS=126;;
        *)    	echo "$SH_NAME : Invalid device name $DEVROOT"
	      	exit;;
    esac
fi
if [ $DISK -eq 0 ] 
then
	DEVPATH="/dev/nr${DEVROOT}"
else
: strip off the partition letter. we always want the raw floppy.
	DEVPATH="/dev/r`echo ${DEVROOT} | sed -e 's/[^0-9]$//'`"
fi
: XXX CRYPTKIT end

: if the front-end "extract_unbundled" hasn't been run do the following
if [ $Pre -eq 0 ]
then
	echo ""
	echo "** Please mount the release media if you haven't done so already. **"
	if [ $DEFAULT -eq 0 ]
	then 
		echo
		echo -n "Press return when ready:"
		read ans
	fi
: Wait for media to load.
	sleep 8
: For Tape devices only since diskettes cannot be used remotely
: Rewind tape, just to be sure
	if [ $REMOTE -eq 1 ]
	then
        	stat=""
		stat=`rsh ${REMOTE_HOST} "$MT -f ${DEVPATH} rew; echo \\$status"`
		case $stat in
			[1-9]* | 1[0-9]* )
				echo
				echo "$SH_NAME : Problem with accessing tape drive, exiting"
				exit 1;;
			*)      continue;;	
		esac
	else
  	    if [ $DISK -eq 0 ]
    	    then
		$MT -f ${DEVPATH} rew
		if [ "$?" -ne  0 ]
		then
			echo
			echo "$SH_NAME : Problem with accessing tape drive, exiting"
			exit 1
		 fi
	    fi
	fi
	# Print copyright to the screen for further verification
	clear
	echo "The following product will be installed:"
	if [ $REMOTE -eq 1 ]
	then
	    rsh ${REMOTE_HOST} $DD if=${DEVPATH} > /tmp/cp.tmp
	    if [ "$?" -ne 0 ]
	    then
		echo
		echo $SH_NAME : Problem with reading copyright file on release tape.
		exit 1
	    fi
	    cat /tmp/cp.tmp
        else
	    if [ $DISK -eq 0 ]
            then 
		$DD if=${DEVPATH} > /tmp/cp.tmp
		if [ "$?" -ne 0 ]
		then
			echo
			echo $SH_NAME : Problem with reading copyright file on release tape.
			exit 1
		fi
		cat /tmp/cp.tmp
	    else
	        $BAR ${OPTIONS} ${DEVPATH} ${BS} +install/copyright.d > /dev/null
                if [ "$?" -ne 0 ]
                then
                    echo $0 : Problem with reading copyright file on release tape.
                    exit 1
                fi                           
                cat +install/copyright.d
		\cp +install/copyright.d /tmp/cp.tmp
		\rm -rf +install
            fi    
	fi
	while [ "$ANS" = "" ]
	do
		echo
		echo -n "Do you want to continue [y|n]? "
		stdin_log ANS
		echo ""
	    case $ANS in
		y*|Y*)	continue;;
		n*|N*)	echo $SH_NAME : Terminating installation...
			exit 0;;
		*)   
			echo 
			echo "Enter \"y\" or \"n\""
			ANS="";;
	    esac
	done
fi
: Set all the unique variables for this product from install_parameters
. ${install_dir}/${PARAM_FILE}
:
:check for correct login for installation
:
case "$REQ_LOGIN" in
	"$INSTALLER" | none)
		;; 
	*) echo
	   echo "$SH_NAME : Incorrect login for installation, you must be $REQ_LOGIN"
	   \rm -f /tmp/cp.tmp
   	   exit 1 ;;
esac
:###################################
:  Display Informational Lists :   #
:###################################
if [ $DEFAULT -eq 0 ]
then
	answer=""
	while [ "$answer" = "" ]
	do
		echo ""
		echo -n "Do you want to see a description of this installation script [y|n]? "
		stdin_log answer
		case $answer in
		y*|Y*)	echo ""
			echo "$SCRIPT_DESC" | fmt
			echo "";;
		n*|N*)	continue;;
		* )  
			echo 
			echo "Enter \"y\" or \"n\""
			answer="";;
		esac
	done
:
: remove any or all of the following 4 displays, if not applicable
:
:	echo "The following are Software Requirements for $PROD:"
:	echo "          $SOFT_REQ"
:	echo "The following are Hardware Requirements for $PROD:"
:	echo "          $HARD_REQ"
:	echo "The following are Optional Software for $PROD:"
:	echo "          $SOFT_OPT"
:	echo "The following are Optional Hardware for $PROD:"
:	echo "          $HARD_OPT"

	echo ""
	echo "Installation should take approximately $INSTALL_TIME."
	echo ""
:##################################
: Display disk usage information  #
:##################################
	echo ""
	echo "Here is the Current Free Disk space:"
        df | egrep '(/dev|Filesystem)'
	echo ""
	echo "This software requires $SOFT_SIZE kbytes of disk space"
	answer=""
	while [ "$answer" = "" ]
	do
		echo ""
		echo -n "Do you want to continue [y|n]? "
		stdin_log answer
	    case $answer in
	    y*|Y*)	continue;;
	    n*|N*)	echo "$SH_NAME : Exiting..."
			\rm -f /tmp/cp.tmp
			exit 0;;
	    * ) 
			echo 
			echo "Enter \"y\" or \"n\""
			answer="";;
	    esac
	done
fi
:##################################################
: Check the OS level compatibility (if necessary) #
: Comment out this section if future OS           #
: compatability is unknown.			  #
:##################################################
for SOS in $SOS_COMPAT
do
	case $SOS_LEVEL in
                ${SOS}* )
: match found
                        SOS_ok=0
                        break;;
                * )
: no match found
                        SOS_ok=1;;
        esac
done
		
: no match was found
if [ $SOS_ok -eq 1 ]
then
	echo ""
	echo "$SH_NAME : This software is not compatible with the current operating system"
	echo ""
	echo "     This is the list of compatible operating systems:"
	for SOS in $SOS_COMPAT
	do
		echo "         $SOS"
	done
	\rm -f /tmp/cp.tmp
	exit 1
fi

:################################## 
: Architecture Compatibility Test #
:##################################
: no match found (not compatible)
ARCH_not_ok=true
for archtype in $ARCH_COMPAT
do
	case $ARCH in
                ${archtype})
			# match found
                        ARCH_not_ok=false
			break
			;;
        esac
done

:###################################################
:  Set up for Standalone or Server installation    #
:##################################################
DEST_LIST=""
MACHINE=""
heter="n"
:##########################################################
: Set up default values if this was run in -DEFAULT mode. #
:##########################################################
if [ $DEFAULT -eq 1 ]
then
	if  ${ARCH_not_ok}
	then
	    echo ""
	    echo "$SH_NAME : This software is not compatible with the $ARCH architecture"
            echo ""
            echo "     This a list of compatible architectures:"
            for archtype in $ARCH_COMPAT
            do
               		echo "         $archtype"
            done
	    \rm -f /tmp/cp.tmp
            exit 1
	fi
	MACHINE="standalone"
	DEST_LIST=$DESTDIR
fi
while [ "$MACHINE" = "" ] 
do
    echo ""
    echo -n "Enter system type [standalone | server]: "
    stdin_log MACHINE
    case "$MACHINE" in
    st* )
	if  ${ARCH_not_ok}
	then
	    echo ""
	    echo "$SH_NAME : This software is not compatible with the $ARCH architecture"
            echo ""
            echo "     This a list of compatible architectures:"
            for archtype in $ARCH_COMPAT
            do
               		echo "         $archtype"
            done
	    \rm -f /tmp/cp.tmp
            exit 1
	fi
	space_test $DESTDIR
	DEST_LIST=$DESTDIR
        STANDALONE="Y" ;;
    se* )
	 servertype=""
         while [ "$servertype" = "" ]
	 do
             echo
             echo -n "Enter server type [homo | heter]: "
             stdin_log servertype
             case "$servertype" in
               ho* )
		    if ${ARCH_not_ok}
		    then
		        echo ""
			echo "$SH_NAME : This software is not compatible with the $ARCH architecture"
		        echo ""
			echo "     This a list of compatible architectures:"
			for archtype in $ARCH_COMPAT
			do
			    echo "         $archtype"
			done
			\rm  -f /tmp/cp.tmp
			exit 1
		    fi
		    space_test $DESTDIR
	  	    DEST_LIST=$DESTDIR
                    break ;;
               he* )
		    heter="y";;
		*)
		    echo ""
                    echo "$SH_NAME: Invalid server type \"${servertype}\"."
		    echo ""
                    echo "Enter \"homo\" or \"heter\""
		    servertype="";;
             esac
         done;;
     * )  
	echo
	echo "$SH_NAME : Invalid system type, use \"standalone\" or \"server\""
	MACHINE="" ;;
    esac
done
:#####################################################################
:  Set up correct Destination Directories for Heterogeneous servers. #
:  Assuming the machine is configured to be a server already	     #
:#####################################################################
: Greater than 4.0, means it will go in /export/exec/sun2|3|4
if [ $heter = "y" ]
then
    if [ "$SOS_LEVEL" = "Sys4" -o $SOS_NUMERIC -lt 40 ]
    then 
	 . ${install_dir}/3.x_install
    else
    for client in $ARCH_COMPAT
    do
	answer=""
	while [ "$answer" = "" ]
	do
		echo ""
		echo -n "Will this be for a $client client [y|n]? "
		stdin_log answer
		case "$answer" in
		  y*|Y* )
			DESTDIR="/export/exec/$client"
			if [ ! -d "$DESTDIR" ]
			then
			    echo ""
			    echo "$SH_NAME : Directory $DESTDIR does not exist"
			    echo ""
			    echo "    This machine does not have a standard server"
			    echo "     configuration for a $client client."
			    echo ""
			    echo "    You can either"
			    echo "    restart for \"standalone\" installation"
			    echo "    or"
			    echo "    make the partition and directory for $DESTDIR"
			    echo "    then restart for \"server\" installation."
			    echo ""
		    	    echo "$SH_NAME : Terminating..."
			    \rm  -f /tmp/cp.tmp
			    exit 0
			fi
			space_test $DESTDIR
			DEST_LIST="$DEST_LIST $DESTDIR";;
		  n*|N* )
			continue;;
		  * )
			echo
			echo "Enter \"y\" or \"n\""
			answer="";;
		esac
	    done
	done
    fi
: If no client was selected default to standalone installation to $DESTDIR
echo "DEST_LIST = $DEST_LIST"
    if [ "$DEST_LIST" = "" ]
    then
	answer=""
	while [ "$answer" = "" ]
	do
	    echo ""
	    echo "No client was selected, installation will default to directory $DESTDIR"
  	    echo -n "Do you want to continue [y|n]? "
	    stdin_log answer
		case "$answer" in
		 y*|Y* )
		    space_test $DESTDIR
		    DEST_LIST="$DESTDIR";;
		 n*|N* )
		    echo $SH_NAME : Terminating installation...
		    \rm  -f /tmp/cp.tmp
       		    exit 0;;
		 * )
		    echo ""
		    echo "Enter \"y\" or\"n\""
		    answer="";;
		esac
	done
    fi
fi
:#####################################################################
:  if Destination directory is changeable and this is a standalone   #
:  installation.  Set up for new Destination directory.              #
:#####################################################################
if [ $DEST_SETABLE = "Y"  -a $STANDALONE = "Y" ]
then
    answer=""
    while [ "$answer" = "" ]
    do
	    echo ""
	    echo "Currently the destination directory is $DEST_LIST"
	    echo "Currently there is $DISK_AVAIL kbytes available in the $DEST_LIST partition"
	    echo -n "Do you want to change the destination directory [y|n]? "
	    stdin_log answer
    	case $answer in
    	y*|Y*)
		echo ""
	        echo "These are the available partitions and free disk space:"
		df | egrep /dev | awk '{print "    " $6 "    " $4}'
		partition=""
		while [ "$partition" = "" ]
		do
		    echo ""
		    echo -n "Enter the partition you would like the software installed in: "
		    stdin_log DESTDIR
		    tmp=`df $DESTDIR 2>/dev/null | egrep /dev`
	    	    if [ "$?" -ne 0 ]
		    then
			echo
		   	echo "$SH_NAME : Nonexistent partition $partition"
		   	partition=""
		    else
			space_test $DESTDIR
			echo
                       	echo -n "Enter the destination directory for the product: $DESTDIR"
                       	stdin_log new_destdir
        	       	if [ -d "$DESTDIR$new_destdir" ]
                       	then
                            DEST_LIST="$DESTDIR$new_destdir"
			else
			    answer=""
			    while [ "$answer" = "" ]
			    do
			        echo
			        echo  "$DESTDIR$new_destdir does not exist;"
			        echo -n "do you want to create that directory [y|n]? "
			        stdin_log answer
				case $answer in
		                y*|Y*)
                               	    mkdir $DESTDIR$new_destdir
                	            if [ "$?" -ne 0 ]
                		    then
				         echo
                        	         echo "$SH_NAME : Failed to create destination directory, check permissions: $DESTDIR$new_destdir"
					 \rm  -f /tmp/cp.tmp
				         exit 1
				    fi
                               	    DEST_LIST="$DESTDIR$new_destdir";;
		                n*|N*)
			 	    new_destdir="";;
				*)  echo
				    echo "$SH_NAME : Enter \"y\" or \"n\""
				    answer="";;
				esac
			    done
			fi
		    fi
	    done;;
	n*|N*) continue;;
	*)  echo
	    echo "Enter \"y\" or \"n\""
	    answer="";;
	esac
    done
fi
:###################################
:  Finally, tar off the software   #
:###################################
for dest_dir in $DEST_LIST
do
    if [ $DEFAULT -eq 0 ]
    then
: Test for pre-existent version of software
        if [ -d "$dest_dir/$SOFT_PRETEST" -o -f "$dest_dir/$SOFT_PRETEST" ]
        then
: XXX CRYPTKIT begin
:               answer=""
:               while [ "$answer" = "" ]
:               do
:                   echo
                    echo "There is pre-existing version of this software."
:                   echo -n "Do you want to overwrite [y|n]? "
:                   stdin_log answer
:                   case $answer in
:                      y*|Y*) continue;;
:                      n*|N*)
                           echo ""
                           echo "$SH_NAME : Terminating Installation"
                           \rm  -f /tmp/cp.tmp
                           exit 0;;
:                      *)
:                          echo
:                          echo "Enter \"y\" or \"n\""
:                          ANS="";;
:                   esac
:               done
: XXX CRYPTKIT end
        fi
	echo ""
	echo "Ready to install $PROD in $dest_dir,"
	ANS=""
	while [ "$ANS" = "" ]
	do
	    echo
	    echo -n "Do you want to continue [y|n]? "
	    stdin_log ANS
	    echo ""
	    case $ANS in
		y*|Y*) continue;;
		n*|N*) 
		    echo
		    echo "$SH_NAME : Terminating Installation"
	    	    exit 0;;
		*)  
		    echo
		    echo "Enter \"y\" or \"n\""
		    ANS="";;
	    esac
	done
    fi
	cd $dest_dir
	if [ $REMOTE -eq 1 ]
	then
		stat=""
		stat=`rsh $REMOTE_HOST "$MT -f $DEVPATH rew; echo \\$status"`
		stat=`rsh $REMOTE_HOST "$MT -f $DEVPATH fsf 2; echo \\$status"`
		if [ "$stat" = "1" ]
        	then
       			echo "$SH_NAME : Error in Extracting Software.  Check Tape."
                        \rm  -f /tmp/cp.tmp
        		exit 1
		fi
		echo "Extracting software... "
        	rsh $REMOTE_HOST $DD if=$DEVPATH bs=${BS}b | ${TAR} xBvfb - ${BS}
		if [ "$?" -ne 0 ]
        	then
       			echo "$SH_NAME : Error in Extracting Software.  Check Tape."
                        \rm  -f /tmp/cp.tmp
        		exit 1
		fi
	else
	    if [ $DISK -eq 0 ]
	    then
		$MT -f $DEVPATH rew
		$MT -f $DEVPATH fsf 2
		echo "Extracting software... "
		$TAR $OPTIONS $DEVPATH $BS
		if [ "$?" -ne 0 ]
		then
       			echo "$SH_NAME : Error in Extracting Software.  Check Tape."
       	 		$MT -f $DEVPATH rewind
                        \rm  -f /tmp/cp.tmp
        		exit 1
		fi
	    else
		$BAR ${OPTIONS} ${DEVPATH} ${BS}
                if [ "$?" -ne 0 ]
                then
                        echo
                        echo $0 : Error in Extracting Software.  Check the media.
			\rm  -f /tmp/cp.tmp
                        exit 1
                fi
		\rm -rf +install
		eject
	    fi
	fi
done
:#######################################################################
: Optional Software : May or may not apply		               #
:   If it exists optional software will be the third file on the tape. #
:   Cannot be used with diskettes since optional software cannot be    #
:   separated on the diskette.					       #
:#######################################################################
if [ "$OPT_REL" = "Y" ]
then
    if [ $DEFAULT -eq 0 ]
    then
	. ${install_dir}/Opt_software
    fi
fi

:#############################################################
: Log Languages Software installation in /usr/lib/lang_info. #
:#############################################################
for install_dir in $DEST_LIST
do
    case "$PROD_NAME" in
    c | lint | pascal | fortran | modula2 )
        TAB="   "
        LANG_INFO=$install_dir/lib/lang_info
        # make an installation log entry in the /usr/lib/lang_info file.
 
        if [ ! -f ${LANG_INFO} ]
        then
                echo "****${TAB}Installing on a pre-4.0FCS system;"
                echo "${TAB}${LANG_INFO} is not present."
                echo "${TAB}Installation aborted."
		\rm  -f /tmp/cp.tmp
                exit 1
        fi
 
        if [ ! -w ${LANG_INFO} ]
        then
                echo "****${TAB}${LANG_INFO} is not writable."
                echo "${TAB}Installation aborted."
		\rm  -f /tmp/cp.tmp
                exit 1
        fi
 
	ex - ${LANG_INFO} <<-!EOF
		g/^${PROD_NAME}${TAB}/d
		\$a
		${PROD_NAME}${TAB}${PROD_RELEASE}${TAB}${PROD_BASE_SOS_RELEASE}
		.
		wq
		!EOF
	;;

    * )
	# nothing to do here.
	;;
    esac
done

if [ $DISK -eq 0 ]
then
    if [ $REMOTE -eq 1 ]
    then
	echo "$SH_NAME : rewinding tape..."
	stat=""
        stat=`rsh -n $REMOTE_HOST "$MT -f $DEVPATH rew; echo \\$status"`
	case $stat in
                [1-9]* | 1[0-9]* )
                        echo "$SH_NAME : Error in tape rewind."
                        exit 1;;
                *)      continue;;
        esac
    else
	echo "$SH_NAME : rewinding tape..."
	$MT -f $DEVPATH rew
	if [ "$?" -ne 0 ] 
	then
        	echo "$SH_NAME : Error in tape rewind."
		exit 1
	fi
    fi
fi
:####################################################
: Log the Software release in the  Inventory list   # 
:####################################################
echo ----------------------------------- >> $UNBUNDLED_INVENTORY
date >> $UNBUNDLED_INVENTORY
: grab only the first part of copyright file w/o copyright info
if [ -f /tmp/cp.tmp ]
then
	head -5 /tmp/cp.tmp >> $UNBUNDLED_INVENTORY
	if [ "$?" -ne 0 ] 
	then
		echo "$SH_NAME : Error in logging software inventory"
		exit 1
	fi
else # this is for the old style 1.X version of extract_unbundled #
    if [ $REMOTE -eq 1 ]
    then
        rsh -n $REMOTE_HOST "$DD if=$DEVPATH" >> $UNBUNDLED_INVENTORY
        stat=`rsh -n $REMOTE_HOST "$MT -f $DEVPATH rew; echo \\$status"`
        case $stat in
                [1-9]* | 1[0-9]* )
                        echo "$SH_NAME : Error in logging software inventory"
                        exit 1;;
                *)      continue;;
        esac
    else
        $DD if=$DEVPATH >> $UNBUNDLED_INVENTORY
        if [ "$?" -ne 0 ]
        then
                echo "$SH_NAME : Error in logging software inventory"
                exit 1
        fi
    fi
fi
\rm -f /tmp/cp.tmp
: work-around to free up the /tmp space used when uncompressing files with
: the bar command.
\rm -f /tmp/tmp_*.Z
:#########################################
:  Log the installation entry and end.   #
:#########################################
echo "$PROD Installation Complete" >> $LOGFILE
echo "$SH_NAME : **** Installation Completed ****"
echo "It is recommended you reboot your system before proceeding."
