#!/bin/sh
#     sccsdiff %Z%%M% %I% %E% SMI     from System III 5.1
#	DESCRIPTION:
#		Execute diff(1) on two versions of a set of
#		SCCS files and optionally pipe through pr(1).
#
# if running under control of the NSE (-q option given), will look for
# get in the directtory from which it was run (if -q, $0 will be full pathname)
#
PATH=$PATH:/usr/sccs

trap "rm -f /tmp/get[abc]$$;exit 1" 0 1 2 3 15

nseflag=
get=get
for i in $@
do
	case $i in

	-*)
		case $i in

		-r*)
			if [ ! "$sid1" ]
			then
				sid1=`echo $i | sed -e 's/^-r//'`
			elif [ ! "$sid2" ]
			then
				sid2=`echo $i | sed -e 's/^-r//'`
			fi
			;;
		-p*)
			pipe=yes
			;;
		-q*)
			nseflag=$i
			get=`dirname $0`/get
			;;
		-[cefnhbwitD]*)
			flags="$flags $i"
			;;
		*)
			echo "$0: unknown argument: $i" 1>&2
			exit 1
			;;
		esac
		;;
	*s.*)
		files="$files $i"
		;;
	*)
		echo "$0: $i not an SCCS file" 1>&2
		;;
	esac
done

if [ ! "$files" -o ! "$sid1" ]
then
	echo "Usage: sccsdiff -r<sid1> -r<sid2> [-p] [-q] sccsfile ..." 1>&2
	exit 1
fi

[ ! "$sid2" ] && current=true

for i in $files
do
	[ "$current" ] && sid2=`$get -g $i`
	if $get $nseflag -s -p -k -r$sid1 $i > /tmp/geta$$
	then
		if $get $nseflag -s -p -k -r$sid2 $i > /tmp/getb$$
		then
			diff $flags /tmp/geta$$ /tmp/getb$$ > /tmp/getc$$
			diffstat=$?
		fi
	fi
	if [ ! -s /tmp/getc$$ ]
	then
		if [ -f /tmp/getc$$ ]
		then
			echo "$i: No differences" > /tmp/getc$$
		else
			exit 1
		fi
	fi
	if [ "$pipe" ]
	then
		pr -h "$i: $sid1 vs. $sid2" /tmp/getc$$
	else
		[ "$diffstat" -ne 0 ] && echo "$i: $sid1 vs. $sid2"
		cat /tmp/getc$$
	fi
done

trap 0
rm -f /tmp/get[abc]$$
