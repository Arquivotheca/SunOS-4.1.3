#!/bin/sh
#%Z%%M% %I% %E% SMI
#
#**NOTE** nawk must be in your path to use this script.
#
# Description:
#	This script is used to select data from the sccslog file.
#
#	sccspar {-f sccslog_file} -p[field 1-6] -s[search string]
#
#
#	Print Fields:
#	-p1	file	:	sundist/SCCS/s.cdimage.mk
#	-p2	rev	:	1.19
#	-p3	date	:	90/05/03
#	-p4	time	:	11:06:44
#	-p5	login	:	psk
#	-p6	bugid(s):	1033852

# Constants:
	BUGID='1[0-9][0-9][0-9][0-9][0-9][0-9]'
	COMMENTSTR='\001c'
	IDSTR='\001d'
	SRCSTR='\/SCCS\/s\.'
	USAGE="usage:  `basename $0` {-f sccslog_file} -p[1-6]... -s[search string]

	-p1     file    :       sundist/SCCS/s.cdimage.mk
	-p2     rev     :       1.19
	-p3     date    :       90/05/03
	-p4     time    :       11:06:44
	-p5     login   :       psk
	-p6     bugid(s):       1033852
	"

# Temporary files:
	RAWTMP=/tmp/rawstuf$$
	SEARCHTMP=/tmp/searchstuf$$
	TMPFILES="$RAWTMP $SEARCHTMP"

# Variables:
	bugid=
	bugsonly=
	date=
	srcfile=
	user=
	sccslogfile=
	select_data_args=

# Functions:

QUIT () {
	rm -f $TMPFILES
	exit $1
}

trap 'echo "`basename $0`: Aborted"; QUIT 1' 1 2 3 15

# raw_stuff1:
#	input to this function is an sccslog file. The output is printed 
#	out as shown to a temporary file $RAWTMP
#
#	sccslog file input
#
# /usr/src/SCCS_DIRECTORIES/suninstall/SCCS/s.add_services.c
# ^Ad D 2.32 90/05/30 12:58:43 psk 62 61
# ^Ac Smerge: D 2.33 90/04/18 16:45:20 lenb 63 62 00000/00001/01629
# ^Ac > Smerge: D 2.32 90/02/02 06:01:00 lenb 62 61       00000/00001/01629
# ^Ac > > PSR and vanilla release share share modules.
# ^Ac > > this delta fixes 1034021: The server is stuck in GMT timezone regardls
# ^Ac > > of suninstall selection
# ^Ac > > *** CHANGED *** 90/02/27 21:12:36 lenb
# ^Ac > > share modules are shared between different realizations
# ^Ac > 4.1_PSR_A -> 4.1.1 (1036965)
#
#	raw_stuff1 output
# suninstall/SCCS/s.add_services.c 2.32 90/05/30 12:58:43 psk 1034021 1036965 
#
raw_stuff1 () {
nawk '
{
if ($1 ~ /'$SRCSTR'/)   #if field 1 matches "/SCCS/s."
	printf"\n%s ", $1
        
if ($1 ~ /^'$IDSTR'/)   #if field 1 matches "^Ad"
	printf"%s %s %s %s ",$3, $4, $5, $6
        
# This statement prints out the bugids. Since the bugid is a string in 
# the sccs comment you never know what is surrounding
# (bugid:1234567,1234568 1234579/1234590) it. So after a bugid is found,
# that string is then split into an array and then goes thru and looks 
# at each character to determine if it is a number and part of a bugid.
#
if ($1 ~ /^'$BUGSTR'/)  #if field 1 matches "^Ac"
	for (i = 1;i <=NF; i++) {
		if ($i ~ /'$BUGID'/) {
			#
			# This is where the string that contains a bugid
			# is massaged by gsub
			# (bug:1234567 becomes  "b u g : 1 2 3 4 5 6 7")
			#
			gsub(/./,"& ",$i)
			#
			# Create the array nums with the elements from
			# gsub $i
			#
			x = split($i,nums)
			#
			# Go thru the loop and print out valid bugids
			#
			bugcount = 0
			for (j = 1; j <= x; j++) {
				if ((nums[j] >= 0) && (nums[j] <=9)) {
					if ((bugcount >=1) && (bugcount <=7)) {
						prtbug[bugcount] = nums[j]
						bugcount++;
					}

					if (bugcount == 0) {
						prtbug[bugcount] = nums[j]
						bugcount++;
					}

					if (bugcount > 7) {
						bugcount = 0
					}

					if (bugcount == 7) {
						for (k = 0; k <= 7; k++) {
							printf"%s", prtbug[k]
						}
						printf" "
						bugcount = 0
					}
				}
				else
				bugcount = 0
			}
		}
	}
}' $1
echo
}

#
# select_data:
#	This function is used to print the data out in the order specified
#	on the command line with the -p option. You can print any of the
#	six fields out in any order.
#
#	input will be from one of the $TMPFILES and will be in the form of:
# 		suninstall/SCCS/s.add_services.c 2.32 90/05/30 12:58:43 psk 1034021
#	and from $select_data_args in the form of:
#		n1 n2... filename 
#	(n1 n2... are field numbers and filename is one of the $TMPFILES
#
#	ouput will be the fields specified and the order in which they are
#	used on the command line.
#
select_data () {
nawk 'BEGIN{
#
# This step will determine the field numbers to be printed. 
for (i = 1; ARGV[i] ~ /^[0-9]+$/; i++) { 
	fld[++nf] = ARGV[i]
	ARGV[i] = ""
}
} # End of BEGIN
{
# This step will print out the fields/order specified
for (i = 1; i <= nf; i++){
	if (fld[i] == 6){
		# Field 6 is the bugid field. Sometimes more than one bugid
		# is entered in the sccs comment. This step determines if more
		# than one bugid exist and then prints them
		for (x = 6; x <= NF; x++) {
			# If the only field requested to print is field 6 (bugids)
			# print a newline after printing each bugid
			if (ARGC == 3 && fld[i] == 6 )
				printf("%s\n", $x)
			else
				printf("%s ", $x)
		}
		printf("%s", i < nf ? "" : "\n")
	}
	else
		printf("%s%s", $fld[i], i < nf ? " " : "\n")
}
}' $select_data_args 
echo
}


# Program

# Declare the valid options
args="$@"
set -- `getopt f:s:p: "$@"` 

[ $? != 0 ] && exit

# Parse the command line 
for i in "$@"
do
	case "$i" in
		#
		-f)	if [ -z "$file" ]
			then
				if [ -f "$2" ]
				then
					file="$2";shift 2
				else
					echo "$0: -f \"$2\" file does not exist"
					exit 1
				fi
			else
				echo "$0 $args"
				echo "$1 $2 can only be used once"
				exit 1
			fi;;
		-s)	if [ -z "$search" ]
			then
				search="$2";shift 2
			else
				echo "$0 $args"
				echo "$1 $2 can only be used once"
				exit 1
			fi;;
		-p)	case "$2" in
				[1-6])	print="$print $2";shift 2;;
				    *)	echo "invalid print field: -p $2";
					echo "$USAGE";
					exit 1;;
			esac;;
		--)	[ -z "$file" ] && {
				echo "$USAGE"
				exit 1
			};
	esac
done

# Get the raw data from the sccslog file
raw_stuff1 $file  | sed -e 's@/usr/src/@@' -e 's@SCCS_DIRECTORIES/@@' > $RAWTMP

if [ -z "$print" ] # Print out all 6 fields and/or data containing search string
then
	if [ -z "$search" ]
	then
		# The sed is to get rid of a blank line at the top of the output
		cat $RAWTMP | sed '/^[ ]*$/d' 
	else
		# The sed is to mask "/" for input input to awk which
		# we are not really using here but may use latter.
		SRCSTR=`echo $search | sed 's@/@\\\/@g'`
		grep "$SRCSTR" $RAWTMP > $SEARCHTMP
		cat $SEARCHTMP | sed '/^[ ]*$/d' 
	fi

else # Print out selected fields and/or data containing search string
	if [ -z "$search" ] 
	then
		select_data_args="$print $RAWTMP"
		select_data | sed '/^[ ]*$/d' 
	else
		SRCSTR=`echo $search | sed 's@/@\\\/@g'`
		grep "$SRCSTR" $RAWTMP > $SEARCHTMP
		select_data_args="$print $SEARCHTMP"
		select_data | sed '/^[ ]*$/d' 
	fi
fi

rm -f $TMPFILES
