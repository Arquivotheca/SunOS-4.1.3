#! /bin/sh -
#
#	%Z%%M% %I% %E% SMI
#

#
# Paranoia.  Set IFS to "<blank><tab><newline>".  Set PATH to
# /bin:/usr/bin:/usr/ucb; this makes sure we don't run anything funny,
# and also makes sure that when we run "ar" we run the REAL "ar".
# We export it to make sure "ranlib" runs the REAL "ar" as well.
#
IFS=" 	
"
PATH=/bin:/usr/bin:/usr/ucb
export PATH

#
# Initialize usage message.
#
usage="usage: ar [mrxtdpq][uvcnbailos] [posname] archive files ..."

#
# Check the number of arguments.
#
if [ $# -lt 2 ]
then
	echo "$usage" 1>&2
	exit 1
fi

#
# Isolate the keyletter argument.
#
keyletters="$1"
shift

#
# Make sure that "a", "b", or "i" was given a "position" argument.
#
case "$keyletters" in

*[abi]*)
	if [ $# -lt 2 ]
	then
		echo $usage 1>&2
		exit 1
	fi
	;;
esac

#
# Throw away leading "-" in keyletter argument.
#
case "$keyletters" in

-*)
	keyletters=`expr "$keyletters" : "-\(.*\)"`
	;;
esac

#
# See if the "s" keyletter was specified.  If so, strip it out, and set
# "doranlib" so the archive will be ranlibbed afterwards.
#
case "$keyletters" in

*s*)
	keyletters=`tr -d 's' <<EOF
$keyletters
EOF`
	doranlib=true
	;;

*)
	doranlib=false
	;;
esac
#
# Do the operation; if it fails, give up.
#
case "$keyletters" in

*[drqm]*)
	doranlib=true
	ar "$keyletters" "$@" || exit $?
	;;

*)
	ar "$keyletters" "$@" || exit $?
	;;
esac

#
# If we are to "ranlib" the archive, do it.
#
if [ "$doranlib" = "true" ]
then
	case "$keyletters" in

	*[bai]*)
		ranlib "$2" || exit $?
		;;

	*)
		ranlib "$1" || exit $?
		;;
	esac
fi
exit 0
