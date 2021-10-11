#!/bin/sh -x
# %Z%%M% %I% %E% SMI
#
# trim an sccs tree down to a release level
#
# We assume that the current directory is the root of an SCCS tree.
# The argument to %M% is an SID-list which specifies a release of
# the source in the SCCS tree. The pathnames in the SID-list must
# resolve properly; i.e., we won't manipulate them, so they must
# work from "here," whereever "here" is.
#
# We rip through the tree and rmdel any deltas "greater than" the delta
# specified in the SID-list.
#
# XXX - What happens to branch deltas?
#
# set up a trap to clean up after ourselves if we are killed...
#
trap "echo $0 interrupted. The SCCS tree is probably corrupt." 2 3 15

#
# who am I?
#
if [ `whoami` != root ]; then
	echo "Error: you must be super-user"
	exit 1
fi

#
# check argument
#
if [ $# -ne 1 ]; then
	echo "Usage: $0 SIDfile"
	exit 1
fi
if [ ! -f $1 ]; then
	echo "Error: SID file $1 does not exist"
	exit 1
fi

#
# now run through the SIDlist
#
egrep -v '^#' $1 | (while read f sid; do
	while true; do
		top=`sccs get -g $f`;
		#
		# the top is greater than the desired SID, so rmdel the top
		#
		if [ `expr $top '>' $sid` -eq 1 ]; then
			sccs rmdel -r$top $f;
			continue;
		fi;
		#
		# if this is true, something's wrong
		#
		if [ `expr $top \< $sid` -eq 1 ]; then
			echo SID $sid of $f not in SCCS tree;
		fi;
		#
		# XXX - continue on anyway?
		#
		break;
	done;
done)
exit 0
