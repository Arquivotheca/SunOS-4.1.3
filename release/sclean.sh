#!/bin/sh
# %W%
#
# This script recursively runs "sccs clean" to remove source files that
# have corresponding SCCS files. Sclean will print out the name of each 
# directory as it is processed. If any source files are being edited
# a message will be printed to that effect and the file will not be
# removed (even if the file is being edited on a different machine.)
#

( echo .;find	`ls -a | sed -e /SCCS/d  -e '/^\.$/d' -e '/^\.\.$/d'` \
	! -name SCCS -type d -print ) | while read d
do
	if [ -d $d/SCCS/ ]
	then
		echo $d
		( cd $d ; sccs clean )
	fi
done
