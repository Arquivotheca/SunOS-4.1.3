#!/bin/sh
#%Z%%M% %I% %E% SMI

# Description:
#	Generates a SCCS history report using the output from sid_cmp
#	Output from this report is the same as /usr/adm/sccslog

# Scripts called by this script:
#	none


# Arguments Descriptions:

# Constants
	PATH=/usr/release/bin:/usr/ucb:/usr/bin:/usr/etc:/usr/local/bin:/usr/scc

# Variables:
	INPUT=$1

# Functions:


# Program:
[ -z "$INPUT" ] && {
	echo "usage: $0 <sid_compare file>"
	exit 1
}

[ -f "$INPUT" ] || {
	echo "Error: input file \"$INPUT\" does not exist"
	exit 1
}


[ -z "$SCCSDIR" ] && {
	echo "Must setenv SCCSDIR <Top of SCCS tree>"
	echo -n "SCCSDIR=/usr/src/SCCS_DIRECTORIES [y/n]: "
	read ans
	case "$ans" in
		y)	SCCSDIR=/usr/src/;break;;
		n)	echo -n "SCCSDIR=";
			read SCCSDIR;
			[ -z "$SCCSDIR" ] && exit 1;
			break;;
		*)	exit 1;;
	esac
}

SCCSDIR="${SCCSDIR}/";

nawk '/SCCS\/s./{

	# input to this function is:
	#
	# 1.14    1.16     usr.bin/ex/SCCS/s.ex.c
	#
	oldsid = $1
	newsid = $2
	file   = $3
	path   = "'$SCCSDIR'"


	# If oldsid = none then this is a new file. Print the entire sccs 
	# history skip the rest of the intructions and process the next entry
	if (oldsid == "none")
	{
		print file
		system ("sccs prs " path file)
		next
	}

	# If newsid = none then the file has been obsoleted. Print message and
	# skip the rest of the intructions and process the next entry
	if (newsid == "none")
	{
		print "obsoleted: "file
		next
	}

	# If we got this far then we need to split the [Level].[Revision] (1.6)
	# into the arrays oldrev and newrev so we can process each part of the
	# SID revision for the rest of the script.
	old = split(oldsid,oldrev,".")
	new = split(newsid,newrev,".")

	# There should only be two parts to the array "Level and Revision"
	# if not 2 parts skip the rest of the intructions and process the 
	# next entry
	if (old != "2" || new != "2")
	{
		print "                                old	new	file"
		print "[Level].[Revision] out of sync: "oldsid"	"newsid"	"file
		next
	}

	# The first element of the arrays oldrev and newrev contain the SID
	# Level. If these are not the same then print messages and skip the
	# rest of the instructions and process the next entry.
	# NOTE: This statement will be latter expanded to handle this situtation
	if (oldrev[1] != newrev[1])
	{
		printf("Error: Sid LEVELS are not consistent\n")
		printf("%-12s%-12s%s\n","old","new","file")
		printf("%-12s%-12s%s\n\n",oldsid,newsid,file)
		next
	}

	# If the newrev is less than the oldrev than somone has done a rmdel
	# or replace the original s.file with another
	if (newrev[2] <= oldrev[2])
	{
		printf("Error: Rmdel has occured\n")
		printf("%-12s%-12s%s\n","old","new","file")
		printf("%-12s%-12s%s\n\n",oldsid,newsid,file)
		next
	}

	# If we made it this far then our SID revision is a valid one and we
	# can print the sccs history of changes made since oldrev
	for (i = newrev[2]; i > oldrev[2]; i--)
	{
		rev = newrev[1]"."i" "
		print file
		system ("sccs prs -r"rev path file)
	}

}' $INPUT 
