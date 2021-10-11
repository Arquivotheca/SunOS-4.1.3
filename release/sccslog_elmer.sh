#!/bin/sh
#%Z%%M% %I% %E% SMI

# Description: sccslog_elmer
#
#	Finds bugids in the sccslog specified on the command line and
#	prints out a 1 line synopsis from bugtrac followed by a list
#	of files modified under that bugid as logged in the sccslog file.
#
#	Example output:
#
#	Bugid   Resp     S/P   Synopsis
#	1042908 geri     5/5   formatting and style errors in man pages
#	jah 90/10/04 16:50:32 man/man8/SCCS/s.unixname2bootname.8 1.3
#	jah 90/10/04 18:47:16 man/man8/SCCS/s.dump.8 1.39
#
#	1031926 jke      2/3   Nihon-go keyboard layout needs to be added to 4.1
#	glennw 90/10/04 13:52:15 usr.etc/loadkeys/SCCS/s.Makefile 1.6


# Scripts called by this script:
#	/elmer/bugtraq/bin/shortbug
#	sccspar


# Argument Descriptions:
#	-f	sccslog filename "/usr/adm/sccslog"

# Constants:
	USAGE="usage:  `basename $0` {-f sccslog_file} "

# Functions:

# Input for print_bug comes from the output of shortbug (Elmer bugtrac report
# command)
# Example of shotbug input to print_bug is as follows:
#
#		Field 1    Field 2
#	Line1	Bug Id:     1038691
#	Line2	Synopsis:  lockscreen -e can have adverse effects
#	Line3	Severity:  2
#	Line4	Priority:  2
#	Line5	Category:  sunview1
#	Line6	Subcategory:  program
#	Line7	Responsible Manager:  jpi
#	Line8	Bug/Rfe:  bug
#	Line9	State:  evaluated
#	Line10	Release: 4.1.1-beta2, 4.1
#	Line11	Submitter: marth	Date:	05/17/90
#
# Example of of print_bug output:
#
#	Bugid   Resp     S/P   Synopsis (** not part of print_bug ouput)
#	1038691 jpi      2/2   lockscreen -e can have adverse effects
#
print_bug () {
	nawk -F: '
		{
		# load field 2 of the entire input to the array line[NR]
		line[NR] = $2
		}

		END {
			
		# The current order is to print field 2 of 
		# lines "1, 7, 3, 4, 2"
		# This is done by setting NR = "to the line number"
		# remove all leading blanks  gsub(/^[ ]*/, "", line[NR])
		#
		# print_bug_hd
		NR = 1
		gsub(/^[ ]*/, "", line[NR])
		printf("%-8s", line[NR])

		NR = 7
		gsub(/^[ ]*/, "", line[NR])
		printf("%-8s ", line[NR])

		NR = 3
		gsub(/^[ ]*/, "", line[NR])
		printf("%s/", line[NR])

		NR = 4
		gsub(/^[ ]*/, "", line[NR])
		printf("%s   ", line[NR])

		NR = 2
		gsub(/^[ ]*/, "", line[NR])
		printf("%s\n", line[NR])

	}'
}


get_bug () {
	sccslogfile="$1"
	nawk 'BEGIN{
		printf("%-8s%-8s %-3s   %s\n",
			"Bugid","Resp","S/P","Synopsis")
	}' /dev/null
	for i in `sccspar -f$sccslogfile -p6 | sort -u`
	do
		shortbug $i  | print_bug
		sccspar -f$sccslogfile -p5 -p3 -p4 -p1 -p2 -s$i
		echo
	done
}


# Program:
args="$@"
set -- `getopt f: "$@"`

if [ $# = 0 ]
then
	echo "$USAGE" | more
	exit
fi

for i in "$@"
do
	case "$i" in
		-f)	if [ -z "$file" ]
			then
				if [ -f "$2" ]
				then
					file="$2";shift 2
				else
					echo "`basename $0` $args"
					echo "error: file \"$2\" does not exist"
					exit 1
				fi
			else
				echo "`basename $0` $args"
				echo "error: $1 $2 can only be used once"
				exit 1
			fi;;
		--)	[ -z "$file" ] && {
				echo "$USAGE"
				exit 1
			};;
	esac
done

get_bug $file
