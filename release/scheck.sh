#!/bin/sh
# %W%
#
# This command checks for SCCS files that have not been used by a build. It
# is intended to find obsolete files and to point out files that SHOULD have
# been used by makefiles but were not.
#
# To use it, cd to the part of /usr/src that you want to check and type
# scheck. Any SCCS file in the sub-tree that is not used will be printed on
# stdout.
#

set -u
umask 2
here=`pwd`/
node=`echo $here | sed -e 's=\(.*usr/src\).*=\1='`
subdir=`echo $here | sed -e 's=\(.*usr/src\)\(.*\)=\2='`
( cd $node/SCCS_DIRECTORIES$subdir ; find . -name 's.*' -print ) |
while read file
do
	regfile=`echo $file | sed 's=SCCS/s.=='`
	if [ ! -f $regfile ]
	then
		echo $file
	fi
done
