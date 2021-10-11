#!/bin/sh
# %Z%%M% %I% %E% SMI
#
# for an entire source tree, take a snapshot of the current SIDs
# 
Usage='snapshot [ -fsv ] release_name'
#	-f	force overwrite of existing SID file
#	-s	report files still out
#	-v	verbose output
#
# We assume that we start out in a directory (presumably /usr/src)
# with the subdirectory 'SCCS_DIRECTORIES' which contains the SCCS source
# in the form:
#
#	...
#	stuff
#	stuff/SCCS
#	stuff/SCCS/(SCCS source files for stuff...)
#	stuff/morestuff
#	stuff/morestuff/SCCS
#	stuff/morestuff/SCCS/(SCCS source files for morestuff....)
#	...
# 
# Snapshot takes as an argument a string specifying the release_name.
# Its output is a file named release_name.SID.
#
#################

VERBOSE=false
FORCE=false
STILL=false

set -- `getopt fsv $*`
if [ $? != 0 ]; then
	echo $Usage >&2
	exit 1
fi
for i in $*; do
	case $i in
	-f)	FORCE=true; shift;;
	-s)	STILL=true; shift;;
	-v)	VERBOSE=true; shift;;
	--)	shift; break;;
	esac
done

if [ $# -ne 1 ]; then
	echo "Error: no release name specified; $Usage" >&2
	exit 1
fi

RELEASE=$1
SID_FILE=`pwd`/$RELEASE.SID
STILL_OUT=`pwd`/$RELEASE.still_out
MAJOR=`pwd`/major_list

if [ -f $SID_FILE -a $FORCE = false ]; then
	echo "Error: SID file for release $RELEASE already exists" >&2
	exit 1
fi

cd SCCS/..
if $STILL; then
	if $VERBOSE; then echo "Looking for files still out..."; fi
	find . -name 'p.*' -exec echo -n {} '' \; -exec cat {} \; | \
	    sed -e 's/.\///' -e 's/SCCS\/p\.//' > $STILL_OUT
	out=`cat $STILL_OUT | wc -l`
	if [ $out -ne 0 ]; then
		echo "Warning: $out files still checked out!" >&2
		echo "List can be found in $STILL_OUT" >&2
	else
		rm -f $STILL_OUT
	fi
fi

if $VERBOSE; then echo "Taking snapshot of release $RELEASE..."; fi
# set up a trap to clean up after ourselves if we are killed...
trap "rm -f $SID_FILE; exit 100" 2 3 15

if $VERBOSE; then
	echo "Finding sub-directories in SCCS directories...."
fi
find . -type d -print | sed -e '/lost+found/d' \
	-e '/^.$/d' \
	-e '/SIDlist/d' > $MAJOR

if $VERBOSE; then echo "Finding SIDs...."; fi
echo "# +Z++M+ +I+ +E+" | tr '+' '%' > $SID_FILE
echo "#" >> $SID_FILE
echo "# SID list for $RELEASE release" >> $SID_FILE
echo "#" >> $SID_FILE
echo "SIDlist/SCCS/s.+M+ +I+" | tr '+' '%' >> $SID_FILE
if $VERBOSE; then
	echo "Directories: `grep "SCCS$" $MAJOR | wc -l` Processed so far:"
fi
dirs=0
for SCCS_DIR in `grep 'SCCS$' $MAJOR`
do
	if $VERBOSE; then echo -n "$dirs  "; fi
	for FILE in $SCCS_DIR/s.*
	do
		SHORT=`echo $FILE | sed 's/^.\///'`
		if [ -f $FILE ] && SID=`/usr/sccs/get -g $FILE` ; then
			echo $SHORT $SID >> $SID_FILE
		fi
	done
	dirs=`expr $dirs + 1`
done
rm -f $MAJOR
if $VERBOSE; then echo "Done...."; fi
exit 0
