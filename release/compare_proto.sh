#! /bin/sh
#
#%Z%%M% %I% %E% SMI
#
# This utility provides a standardized way of creating proto lists
# that can be 'diffed' for any purpose.  It also facilitates 
# verification of deliverables by comparing two releases to 
# identify the following:
#
#	o New or Removed files
#	o Change in type of files (eg: plain file/link/dir...)
#	o Change in Permission
#	o Change in Ownership
#	o Change in Path
#
# When you invoke this script to generate a new proto list, you
# have the option of continuing with the 'compare function' using
# another list from a previous release.  The identifiers you provide 
# for each list will be prepended to the diff'd lines, to make the 
# final output more readable (Keep them short ~5 chars!).  It will 
# also be used to name the final output of this utility.
#
# The following restrictions apply:
#
# o Lists to be compared should have been generated either by this
#   utility, or the following 'find' command:
#
#	find . -depth -exec ls -ldg {} \; > $protolist
#
# o Existing lists to be processed should reside in the directory
#   /usr/krypton/records/protos, to conserve disk space and
#   encourage sharing of release information among REs from 
#   other divisions.  (Make sure your list has read-only perms)
#
# o For meaningful results, you should generally compare lists 
#   for the same architecture.
#
# This script is based on general procedures supplied by Brent Callaghan
# of SPD.  I have merely combined them into a cohesive, easy-to-use 
# Release Engineering tool.

# Please send bugs/RFE's to janet@firenze WSD Release Engineering

here=`pwd`
tmp=/tmp
listdir=/usr/krypton/records/protos
new_proto=

#
#Screen yes/no reponses
#
get_yn='(while read yn
do
	case "$yn" in
		[Yy]* ) exit 0;;
		[Nn]* ) exit 1;;
		    * ) echo -n "Please answer y or n:  ";;
   esac
done)'
#
#Need to be root to read uucp files in proto
#If generating proto list, ask if 'compare' function desired.
#
while echo -n "Create proto list?  (y/n) "; do
	eval "$get_yn" 
	case $? in
		0) new_proto=1
		   [ ! `whoami` = "root" ] && \
		   echo "Need to be superuser to read uucp files." && exit 1
		   echo -n \
"After creating proto list, compare it with another list? (y/n)  "
		   eval "$get_yn" || list_only=1
		   break ;;
		1) new_proto= ; break;;
	        *) ;;
	esac
done
#
#Identifiers are used to make 'diff' output more readable
#
if [ ! "$list_only" ]; then
	echo -n "Enter short identifier for NEW list (eg:alpha beta):  "
	read vers1
	filter1=${tmp}/${vers1}.filter
	while echo -n \
	"Enter short identifier for PREVIOUS list (eg:alpha beta):  "; do
		read vers2
		[ "$vers2" = "$vers1" ] && \
		echo "NEW and PREVIOUS are the same!" && continue
		filter2=${tmp}/${vers2}.filter && break
	done

	echo "These lists currently exist in $listdir:
	"
	ls -lt $listdir|more
	while echo -n "Enter filename for '$vers2' list: $listdir/";
	do
		read oldlist
		[ -f $listdir/$oldlist ] && oldproto=$listdir/$oldlist && break
		echo "$listdir/$oldlist not found."
	done
fi

if [ "$new_proto" ]; then
	while echo -n "Is your proto area /proto? (y/n)  "
	do
		eval "$get_yn" && protodir=/proto && break
		echo -n "Enter path of your proto area:  "
		read protodir
		[ -d $protodir ] && break
		echo "Directory $protodir not found!"
	done
	release=`cat $protodir/usr/share/sys/conf.common/RELEASE`
	protolist=$tmp/${release}_`arch -k`

	trap "echo $util ABORTED! | tee -a $protolist; exit 1" 1 2 3 15
	echo "Creating proto list; this will take about 20 minutes..."
	echo "***(`arch -k`)`hostname`:$protodir***`date '+%m/%d/%y'`***" >\
	$protolist
	cd $protodir ; find . -depth -exec ls -ldg {} \; >> $protolist
	chmod 444 $protolist
	new_proto=1
	echo "Complete proto list is in $protolist."
	[ "$list_only" ] && exit
fi
trap "echo $util ABORTED!; exit 1" 1 2 3 15

if [ ! "$new_proto" ]; then
	while echo -n "Enter filename for '$vers1' list: $listdir/"; do
		read newlist
		[ -f $listdir/$newlist ] && protolist=$listdir/$newlist && \
		break
		echo "$listdir/$protolist not found."
	done
fi

#
#Filter removes extraneous fields such as file size, date
#
for i in 1 2; do
	if [ "$i" = 1 ] ; then
		 input=$protolist
		 output=$filter1
	else
		 input=$oldproto
		 output=$filter2
	fi
	echo "Filtering $input..."
	cd $tmp; 
	echo "	Filtered output is $output."
	awk '{printf "%s %-8s %-8s %s\n", $1, $3, $4, substr($9,3)}'\
	$input | sort -b +3 > $output
done
#
#Diffs can be any of:  filetype/perm/own/group/path/filename
#
diffs=$tmp/${vers2}_${vers1}_diffs
echo "Comparing protolists..."
diff $filter2 $filter1 > $tmp/$$
sed '/^[0-9]/d' $tmp/$$ | \
sed -e "s/^>/   ${vers1}:  >/g" -e "s/^</${vers2}: </g" > $diffs
echo "Output of 'diff' is $diffs." 
[ "$new_proto" ] && echo "Save $protolist in $listdir for future use."
rm -f $tmp/$$
cd $here

