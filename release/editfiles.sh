:
# @(#)editfiles.sh	1.1
# This script finds sccs files that are being sccs edited in
# the "/usr/src/SCCS_DIRECTORIES", and returns their full
# path plus user ID.
#

SCCSDIR="/usr/src/SCCS_DIRECTORIES"

for i in `find $SCCSDIR -name 'p.*' -print | sed 's:/SCCS/p.*::'`;
	do 
		if [ "$i" != "$ii" ]; then
			echo $i":";sccs info $i;echo "";
		fi; ii=$i
	done
