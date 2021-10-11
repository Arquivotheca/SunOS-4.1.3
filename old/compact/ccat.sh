#! /bin/sh
#
#	@(#)ccat.sh 1.1 92/08/05 SMI; from UCB 4.1 83/02/11
#
for file in $*
do
	/usr/old/uncompact < $file
done
