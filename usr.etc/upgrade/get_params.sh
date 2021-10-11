#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# NAME: get_params
#
# gets parameters from the user
# sets the following variables:
# 	DEVROOT, DEVPATH, PARAMS, REMOTE, REMOTE_HOST, MEDIA
#

for param in $*;
do
    PARAMS="$PARAMS $param"
    case "$param" in
    -d*)
        DEVROOT=`$EXPR $param : '-d\(.*\)' '|' "1"`
        if [ $DEVROOT -eq 1 ]
        then
            echo
            echo "$CMD: Invalid device name"
            echo "            USAGE : $USAGE"
            cleanup 1
        fi
        ;;
 
    -r*)
        REMOTE_HOST=`$EXPR $param : '-r\(.*\)' '|' "1"`
        if [ $REMOTE_HOST -eq 1 ]
        then
            echo
            echo "$CMD: Invalid Host name"
            echo "            USAGE : $USAGE"
            cleanup 1
        fi
        REMOTE="yes"
        ;;
 
    *)
        echo
        echo USAGE : $USAGE
        cleanup 1
        ;;
    esac
done

# determine local vs. remote tape drive

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
                echo "$CMD: Problem with reaching remote host $REMOTE_HOST"
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
    echo "Using a remote drive..."
else
    echo "Using a local drive..."
fi


# get device from user

while [ ! "$DEVROOT" ]
do
    get_ans DEVROOT 'Enter Device Name (e.g. st0, mt0, sr0)'

    case "$DEVROOT" in
    st*|mt*) MEDIA="tape" ; ;;
    sr*)     MEDIA="cdrom" ;;
    *)       echo "$CMD: Invalid device name $DEVROOT"
             DEVROOT=""
             ;;
    esac

    if [ "$DEVROOT" ]
    then
        PARAMS="$PARAMS -d$DEVROOT"
    fi
done
 
# create the device path
 
case "${MEDIA}" in
    "tape")   DEVPATH="/dev/nr${DEVROOT}" ;;
    "cdrom")  DEVPATH="/dev/${DEVROOT}" ;;
    *)        echo "$CMD: Invalid device name $MEDIA"
              cleanup 1
              ;; 
esac
 
echo ""
echo "Using device $DEVPATH..."


