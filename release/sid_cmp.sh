#!/bin/sh
#%Z%%M% %I% %E% SMI

# Description:
#	sid_cmp takes two arguments. These two args are names of sidlists.
#	If you plan to use the output of this script for input to sid_history
#	you must put the oldest sidlist as arg1 and the newer sidlists as arg2.
#
#	For example:
#		sid_cmp 4.1.1_ALPHA1.1.SID 4.1.1_BETA1.0.SID

# Constants:
        PATH=/usr/release/bin:/usr/ucb:/usr/bin:/usr/etc:/usr/local/bin:/usr/sccs
	USAGE="usage: $0 <oldsidlist> <newsidlist>"
	export PATH
	diffoutput=/tmp/siddiffs

# Functions:

QUIT () {
	rm -f $diffoutput
	rm -f /tmp/sidsort1 /tmp/sidsort2
        exit $1
}        
 
trap 'echo "`basename $0`: Aborted"; QUIT 1' 1 2 3 15


# Program:

# Make sure 2 args are supplied on the command line
[ $# = 2 ] || {
	echo $USAGE
	exit 1
}

# Make sure sidlists exist
[ -f $1 ] || {
	echo "Error: \"$1\" does not exist"
	exit 1
}
[ -f $2 ] || {
	echo "Error: \"$2\" does not exist"
	exit 1
}

# Remove old temp file
[ -f $diffoutput ] && rm -f $diffoutput

# Sort'em before diff'in um
sort $1 > /tmp/sidsort1
sort $2 > /tmp/sidsort2

diff /tmp/sidsort1 /tmp/sidsort2 > $diffoutput
files=`awk '/SCCS/{print $2}' $diffoutput | sort -u`

for i in $files
do
	file=`echo $i | sed 's@/@\\\/@g'`
	awk 'BEGIN{
		sid1 = "none"
		sid2 = "none"
		}
		{
		if ($0 ~ /< '$file' /)
			sid1 = $3

		if ($0 ~ /> '$file' /)
			sid2 = $3

		next

		}END{
		print sid1"	"sid2"	'$i'"

	}' $diffoutput
done

# Clean up tmp files and exit status 0
QUIT 0
