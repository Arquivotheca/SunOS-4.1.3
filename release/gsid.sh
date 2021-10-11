#! /bin/sh
#%Z%%M% %I% %E% SMI
#
# Description:
#	This script will checkout all files from a sid-list
#	specified on the command line. 



# Variables:

	USAGE=" usage: $0 [-y \"set noninteractive\"] <-s sid-list>"
	SID=
        AUTOYES=FALSE
        HERE=`pwd`

gsid_create () {

        [ "$AUTOYES" = "FALSE" ] && {
                while true
                do
                        echo -n "Checkout $SID sid-list [y/n]: "
                        read ANSWER
                        case "$ANSWER" in
                                y)      break;;
                                n)      exit;;
                                *)      ;;
                        esac
                done
                }

	echo "`date`" | tee $HERE/LOG$$
	echo "Checkout of `basename $SID` sid-list." | tee -a $HERE/LOG$$
	sed 's/SCCS\/s\.//g' $SID > $HERE/SID$$

	while read file rev
	do
        	cd $HERE/`dirname $file`
        	echo -n "$file  " | tee -a $HERE/LOG$$
        	sccs get -r$rev `basename $file` | tee -a $HERE/LOG$$
	done <  $HERE/SID$$
	echo "`date`" | tee -a $HERE/LOG$$
}

# Program:

        [ $# = "0" ] && {
                echo "$USAGE"
                exit
                }   

        set -- `getopt ys: "$@"`
        for i in "$@"
        do
                case "$i" in
                        -y)     AUTOYES=TRUE;shift 1;;
                        -s)     SID="$2";shift 2;
                                [ -f $SID ] || {
                                        echo "$SID sid-list does not exist."
                                        exit
                                };;
                        --)     break;;
                esac
        done

        gsid_create
