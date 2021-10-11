#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# NAME: tape_subs
#
# These routines deal with the tape. 
# There is a matching routine in cdrom_subs.


# initialize tape

media_start() {
	return
}


# rewind tape

media_rewind() {
    	if [ "$REMOTE" ]; then
            stat=""
            stat=`rsh -n "${REMOTE_HOST}" \
                "$MT -f ${DEVPATH} rew; echo \\$status"`
            case $stat in
            [1-9]* | 1[0-9]* )
            	echo
            	echo "$CMD: Problem with seeking tape drive, exiting"
            	cleanup 1
            	;;
            *)
            	;;
            esac
    	else
       	    $MT -f ${DEVPATH} rew
       	    if [ $? -ne 0 ]; then
            	echo
            	echo "$CMD: Problem with seeking tape drive, exiting"
            	cleanup 1
            fi
        fi
}


# skip the tape forward 
#
# 	$1 is the number of files to skip

media_skip() {
    echo "Positioning the tape to file $1..."
 
    if [ "$REMOTE" ]
    then
        stat=""
        stat=`rsh -n "$REMOTE_HOST" "$MT -f ${DEVPATH} asf $1; echo \\$status"`
        case $stat in
        [1-9]* | 1[0-9]* )
            echo
            echo "$CMD: Problem with seeking tape drive, exiting"
            cleanup 1
            ;;
        *)
            ;;
        esac
    else
        $MT -f ${DEVPATH} asf $1
        if [ $? -ne 0 ]
        then
            echo
            echo "$CMD: Problem with seeking tape drive, exiting"
            cleanup 1
        fi
    fi

}

# extract tar file from tape

media_extract() {
        media_skip "$U_FILE"

        if [ "$REMOTE" ]; then
            rsh -n  "$REMOTE_HOST" "$DD if=$DEVPATH bs=40b" |
            ${uncompress} | $TAR xBvfp - `cat $list` 
            stat=$?
        else
  	    # this needs to be in a subshell to workaround a tape bug
            ( dd if=${DEVPATH} bs=40b | ${uncompress} | 
              $TAR xvfp - `cat ${list}` )
	    wait
	    stat=$?
        fi

        if [ $stat -ne 0 ]; then
            echo
            echo "$CMD: Extraction failed!"
            echo
            cleanup 1
	fi
}

# check the current volume of the tape, and tell user to mount
# the correct volume if necessary
#
# 	$1 is the volume needed

media_check() {
   	tape_ok=""
    	while [ ! "$tape_ok" ]
    	do
	    # sets variables VOLUME, ARCH, RELEASE
       	    toc_release_info 

            if [ "$VOLUME" != "$1" ]; then

                echo ""
               	echo "You currently have release volume $VOLUME mounted."
               	echo "Please mount release volume $1 instead."

               	return_when_ready

               	extract_toc
            else
               	tape_ok=yes
            fi
        done
}


# extract toc from tape

extract_toc() {

	echo ""
        echo "Examining table of contents on release media..."
 	media_rewind
	media_skip 1
 
        command="$DD if=${DEVPATH}"
 
        [ "$REMOTE" ] && command="rsh -n "${REMOTE_HOST}" $command"
 
        eval `$command | ${XDRTOC} > ${TOCFILE}`
}
