#! /bin/sh
#%Z%%M% %I% %E% SMI
#
#Remarks:
#	This script generates a bugtraq report containing the
#	following fields:  
#
#  	   bugid, synopsis, severity, priority, category and manager
#
#	You may copy this script to elmer and execute it from there,
#	or from any machine which has the proper bugtraq directories
#	mounted.  This script depends on the existence of an input file
#	which lists the bugids to be processed, separated by newline 
#	or white space.  (See Suggested use, below)
#	
#Suggested use:
#	To expand on bugfix info contained in the sccslogs of an
#	RE integration machine, do the following:
#
#		a) Run another RE tool "do_sccs_report" (select #1)
#		b) Use the output of a) as input to this utility.
#
#Please send bugs/RFE's to Janet Timbol (janet@firenze)
#

TMP=/tmp
GET_YN='(while read yn; do
	case "$yn" in
	   [Yy]* ) exit 0;;
	   [Nn]* ) exit 1;;
	       * ) echo -n "Please answer y or n:  ";;
     	esac
done)'

while echo -n "
	  
	1)	Perform short query on elmer
	2)	Generate final report
	3)	Do both 1) and 2)

Choose 1|2|3:  "; do
read CHOICE
	case $CHOICE in
	   1)	DO_QUERY=1
		if [ ! `hostname` = "elmer" ]; then
		   mounts=`/etc/mount |egrep "^elmer:/export/bugtraq" |wc -l`
		   [ $mounts -ne 3 ] && err=1 
		   break
		fi;;
	   2) DO_FINAL=1; break;;
	   3) DO_QUERY=1; DO_FINAL=1; break;;
	   *) echo "";;
	esac		
done

if [ "$err" ] ; then
	echo "Please log on to elmer or make sure you have the"
	echo "proper bugtraq mounts on '`hostname`'."
	exit 1
fi

if [  "$DO_QUERY" ]; then
	OUT=bugs.short
	ERR=$TMP/bqerrs_$$; rm -f $ERR
	if [ -s $OUT ] && egrep -is submitter $OUT; then
	   echo -n "Output file $OUT currently exists.  Clobber? (y/n) "
	   eval "$GET_YN"
	   if [ "$?" = 1 ] ; then
	      echo "Please save it and reinvoke this script." && exit 1
	   fi
        fi
	rm -f $OUT
	while echo -n "Enter filename containing bugids:  "; do
		read FNAME
		[ ! -f $FNAME ] && echo "$FNAME does not exist!" || break
	done

	IDLIST=$TMP/idlist_$$
	egrep "^[0-9]*[0-9]" $FNAME > $IDLIST
	echo "Performing bugtraq queries (may take a while...)"
	echo "OUTPUT file:  $OUT"
	for i in `cat $IDLIST`
	do
		echo "====================================================================" >> $OUT
		shortbug $i >> $OUT 
	done
	fgrep ' not found.' $OUT >> $ERR
	if [ -s $ERR ] ; then
		cat $ERR
		echo "Please delete errors from '$OUT' now."
		echo -n "Press <RETURN> to continue:  "
		read ret
		vi $OUT
	fi
fi

if [ "$DO_FINAL" ]; then
	INPUT=bugs.short
	if [ ! -f $INPUT ] ; then
		echo "'$INPUT' does not exist!"
		echo \
		"You should have a buglist, then choose #1 from the menu."
		exit 1
	fi

	OUT=bugreport_$$
	echo "OUTPUT is $OUT"

	awk '   { if ($1 ~ /Bug/ ) printf ("\n%s", $3) }
		{ if ($1 ~ /^Synopsis/ ) printf ("%s", $0) }
		{ if ($1 ~ /^Severity/ ) printf ("\n\tSV= %s", $2) }
		{ if ($1 ~ /^Priority/ ) printf ("\tPR= %s", $2) }
		{ if ($1 ~ /^Category/ ) printf ("\tCATEGORY= %s", $2) }
		{ if ($1 ~ /^Responsible/ ) printf ("\tMGR= %s", $3) } ' \
		$INPUT | sed 's/Synopsis:  //' > $OUT
fi
rm -f $TMP/*$$
echo "DONE"
