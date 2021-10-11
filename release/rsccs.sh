#! /bin/sh
# Name: rsccs - recurse down an SCCS hierarchy applying the
#               specified sccs command
#
# USAGE: rsccs "sccs command" pathname
#
# Example: 
#	 rsccs 'sccs get' /usr/src
#
USAGE="USAGE: rsccs 'sccs command' 'full path'"

[ ! $# = "2" ] && echo $USAGE && exit 1
# verify parameters
if [ ! `expr substr "$2" 1 1` = "/" ]; then
	echo $0: full pathname required.
	exit 1
fi

sccscmd=$1
dir=$2
[ ! -d $dir ] && echo "$dir does not exist!" && exit 1
PWD=`pwd`
cd $dir
#list all first level directories under $dir, excluding *SCCS*
#cd to each subdirectory not named SCCS
#if an SCCS directory exists, execute "$sccscmd SCCS/*"

find `ls -a | sed -e /SCCS/d  -e '/^\.$/d' -e '/^\.\.$/d'` \
           ! -name SCCS -type d  -print | while read i; do
	cd $dir/$i
	if [ -d SCCS ]; then
		for file in SCCS/*; do
			echo $file
			if [ -f ${file} ]; then
				$sccscmd $file
			fi
		done
	fi
done
cd $PWD

