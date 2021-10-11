#!/bin/sh
#%Z%%M% %I% %E% SMI

# Description:
#       Generate a sid list from starting place specified on the command line.
#
#	Example:
#
#	gensid.sh -d /usr/src/SCCS_DIRECTORIES
#	will generate a sidlist of all SCCS files start from -d argument.
#
#	gensid.sh -d /usr/src/SCCS_DIRECTORIES -s sys/sun4c/conf
#	will genarate a sidlist of all SCCS files starting from 
#	sys/sun4c/conf. 

# Scripts call by this script:
#	none

# Argument Descriptions:
#	-d	Top of SCCS tree ie. "/usr/src/SCCS_DIRECTORIES"
#	-s	Start search from this point of the -d argument 

# Constants:
	PATH=/usr/release/bin:/usr/ucb:/usr/bin:/usr/etc:/usr/local/bin:/usr/sccs
	export PATH
	USAGE="	usage: $0 <-d sccs directory path>
			  [-s start from directory below -d path]
			  [-l long listing ]"

# Variables
	SCCSDIR=
	DIR=
	DIRPATH=
	START=
	long_list=FALSE
	

# Functions:

QUIT () {
	exit $1
}

trap 'echo "`basename $0`: Aborted"; QUIT 1' 1 2 3 15

gensid_create() {

	cd ${SCCSDIR}
	DIRPATH=$SCCSDIR
	[ -n "$START" -a -d "$START" ] && {
		DIR="$START/"
		DIRPATH="$SCCSDIR/$START"
		cd $DIR
	}


	for i in `find . -name s\.\* -print | sort | sed 's@^./@@'`
	do
        	echo -n "${DIR}$i "
		if [ $long_list = "TRUE" ]
		then
        		sccs prs -d":R:.:L: :D: :T: :P:" $i
		else
			sccs prs -d":R:.:L:" $i
		fi
	done
}

# Program:

	[ $# = 0 ] && {
		echo $USAGE
		exit 1
	}

	args=$@
	set -- `getopt d:s:l "$@"`

	[ $? != 0 ] && {
		echo $USAGE
		exit 1
	}

	rm -f /tmp/sid$$

	for i in "$@"
	do
		case "$i" in
			-d)	if [ -d "$2" ]
				then
					SCCSDIR="$2";shift 2
				else
					echo "$0 $args"
					echo "error: directory \"$2\" does not exist"
					exit 1
				fi;;
			-s)	if [ -d $SCCSDIR/"$2" ]
				then
					START="$2";shift 2;
				else
					echo "$0 $args"
					echo "error: directory \"$2\" does not exist"
					exit 1
				fi;;
			-l)	long_list=TRUE;shift 1;;
			--)	[ -z "$SCCSDIR" ] && {
					echo "$0 $args"
					echo "error: -d argument required"
					echo $USAGE
					exit 1
				};
				break;;
		esac
	done

	gensid_create 
