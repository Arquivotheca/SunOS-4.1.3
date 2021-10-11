#!/bin/sh
#
#      %Z%  %M%  %I%  %E%  SMI
#
#	Compute disk space used in the by files and subdirectories 
#	in the current directory.  The -i and -e flags can be
#	used to include or exclude files to be considered.  The
#	filename contains a list of file names, one per line.
#	The value reported is padded by at least 2% for the -i
#	and default options. Since du operating on plain file
#	arguments can overestimate, the -e option pads the
#	result by another 5%. This is gauche.

sizeof()
{
	( cd ${DIR:-.} ; du -s $* | awk '{s+=$1};END{printf "%d", s * 1.02}' )
}

case $1 in 
	"-i") shift ; sizeof `cat $*` ;; 
	"-e") shift ; echo `sizeof` `sizeof \`cat $*\`` | \
		awk '{printf "%d", ($1 - $2) * 1.05}';;
	   *) sizeof "$*" ;; 
esac |
echo `cat`kb
