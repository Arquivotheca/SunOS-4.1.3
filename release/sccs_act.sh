#!/bin/sh
#%Z%%M% %I% %E% SMI
# Description:
#	This script will gather information on the sccs tree and generate
#	a report on the previous 24hrs.

# Scripts called by this script:
#	gensid
#	sid_cmp
#	sid_rpt
#	day

# Data files used

# Arguments Descriptions:
#	-d	full path to TOP of SCCS tree
#		Default is 4.1.1 PSR A SCCS tree 
#			/net/valis/usr/src.411_PSRA/SCCS_DIRECTORIES/

# Constants:
	PATH=/usr/release/bin:/usr/ucb:/usr/bin:/usr/etc:/usr/local/bin:/usr/sccs
	PREVDAY=sid`day yesterday`
	SIDDIR=/usr/src/release_info/sidlists
	SIDFILE=${SIDDIR}/sid
	SIDCMP=${SIDDIR}/sid_compare.`day today`
	SIDRPT=${SIDDIR}/sid_history.`day today`
	export PATH SIDDIR SIDFILE SIDCMP SIDRPT PREVDAY

# Variables:
	SCCSDIR="/net/valis/usr/src.411_PSRA/SCCS_DIRECTORIES"
	export SCCSDIR

# Functions:

get_sid_rev () {
	nawk '
	BEGIN {
		RS      = ""
		FS      = "\n"
		FILE    = " '$SIDFILE'"
		NEWFILE = " -G'$PREVDAY'"
	}
	{
		if (($0 ~ "'$PREVDAY'") && ($1 ~ /^[D] /)){
			gsub(/ /,"&\n",$1)
			x = split($1,rev)
			system("sccs get -r"rev[2] FILE NEWFILE)
		}
	}'
}

# Program:

set -- `getopt d: "$@"`
[ $? != 0 ] && exit

# Parse the command line
for i in "$@"
do
	case "$i" in
		-d)	SCCSDIR="${2}/";shift 2; break;;
		--)	shift;break;;
	esac
done


[ -d "$SIDDIR" ] || {
	echo "directory $SIDDIR: does not exist"
	exit 1
}

[ -d "$SCCSDIR" ] || {
	echo "directory $SCCSDIR: does not exist"
	exit 1
}

cd $SIDDIR

gensid -d $SCCSDIR -l > $SIDFILE.tmp
[ $? != 0 ] && exit 1

rm -f sid 
[ $? != 0 ] && exit 1

sccs edit sid
[ $? != 0 ] && exit 1

rm -f sid 
[ $? != 0 ] && exit 1

mv $SIDFILE.tmp $SIDFILE
[ $? != 0 ] && exit 1

sccs delget -y"sid`day today`" sid
[ $? != 0 ] && exit 1

sccs prs sid | get_sid_rev
[ -f $PREVDAY ] || {
	echo "$PREVDAY: does not exist"
	exit 1
}

sid_cmp $PREVDAY  sid > $SIDCMP
[ $? != 0 ] && exit 1

rm -f sid_compare
[ $? != 0 ] && exit 1

sccs edit -s sid_compare
[ $? != 0 ] && exit 1

mv $SIDCMP sid_compare
[ $? != 0 ] && exit 1

sccs delget -s -y"sid_compare_`day today`" sid_compare
[ $? != 0 ] && exit 1

sid_rpt sid_compare > $SIDRPT
[ $? != 0 ] && exit 1

rm -f sid_history
[ $? != 0 ] && exit 1

sccs edit -s sid_history
[ $? != 0 ] && exit 1

mv $SIDRPT sid_history
[ $? != 0 ] && exit 1

sccs delget -s -y"sid_history_`day today`" sid_history
[ $? != 0 ] && exit 1

rm -f sid`day yesterday`
cat sid_compare sid_history | /usr/ucb/mail -s "`date` nightly SIDreport" alison 
