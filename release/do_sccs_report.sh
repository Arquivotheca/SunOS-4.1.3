#! /bin/sh
#
#%Z%%M% %I% %E% SMI
#
# This utility parses various fields from a designated sccslog.
# The named log can be processed in its entirety, or from a
# start date specified by the user.
#
# Duplicate entries are stripped (eg: same source file, edited same day
# by same user to fix same bug)
#
# Suggested uses:
# Report #1: Track bugs fixed for a certain timeframe
# Report #2: Send to developers as a reminder to update bugtraq
# Report #3: Check src changes that look like bug fixes, but have no bug ID
# Report #4: Check src changes that may affect sources previously forked 
#		for another release.
#
# Please send bugs/RFE's to Janet Timbol (janet@firenze)


while echo -n "

These sorted sccslog reports are available:

	1)	Bug IDs 
	2)	Login, BugIDs
	3)	Source, Date, Login, Bugid
	4)	Date, Source, Login, Bugid

Enter report number or (q) to quit: "
do

	read choice
	case x_$choice in
	   x_[Bb1] )	ID=1; OUT=/tmp/id.out;;
	   x_[Ll2] )	LOGIN_ID=1; OUT=/tmp/login_id.out;;
	   x_[Ss3] )	SRC_DATE=1; OUT=/tmp/src_date.out;;
	   x_[Dd4] )	DATE_SRC=1; OUT=/tmp/date_src.out;;
	    x_[Qq]*)	exit;;
	          *) echo -n \
	   "	'$choice' is not a valid choice.  Press <RETURN> to continue:  "
	   read return; continue
	esac
	break
done
TMPFILE=/tmp/get_src.out
rm -f $TMPFILE

while echo -n "Enter full path of sccslog to be processed:  " ; do
	read INPUT
	[ "$INPUT" ] && [ -f $INPUT ] && break
	echo "	$INPUT does not exist!"
done

while echo -n \
"Enter start date (yy/mm/dd) for processing sccslog
OR press <RETURN> to read the entire log:  " ; do
	read R_START
	case x_$R_START in
	x_[89][0-9]/[01][0-9]/[0-3][0-9]) START=`echo $R_START|sed "s/\///g"`
	    NOW=`date '+%y/%m/%d' | sed "s/\///g"`
	    [ $START -gt $NOW ] || break
	    echo "	$R_START is later than today!!";;
        x_) START=""; break;;
	*) echo "	Format should be yy/mm/dd!";;
	esac
		    
done

#
#print intermediate file, one record per line, as follows:
#source date login bugid bugid ....
#duplicate lines are removed

echo "Please wait..."

bugids='[0-9][0-9][0-9][0-9][0-9][0-9][0-9]'

awk '   { if ($1 ~ /\/SCCS\// ) printf ("\n%s ", $1) }
        { if ($1 ~ /d/ ) printf ("%s %s " "", $4, $6) }
	{ if ($1 ~ /c/ )
	i=1
	while (i <= NF) {
	{ if ($i ~ /[0-9][0-9][0-9][0-9][0-9][0-9][0-9]/) 
	printf ("%s " "",  $i) }
	i++ }
    	}
	END {print $0}' $INPUT  | sort | uniq > /tmp/src$$
        awk '{print $2, $0}' /tmp/src$$ | sed '1,$s/\///' | \
            sed '1,$s/\///' | sort > /tmp/src$$.work
	if [ "$START" ]; then
          awk '$1 >= '$START'' /tmp/src$$.work | sed 's/.......//' > $TMPFILE
	else
          [ -s /tmp/src$$ ] && mv /tmp/src$$ $TMPFILE
	fi

[ ! -s $TMPFILE ] && echo "No records found." && exit

NOW=`date '+%y/%m/%d %H:%M:%S'`
echo "Output File is: $OUT"
echo "Input file:  $INPUT" > $OUT
if [ "$START" ]; then
   echo "processed from $R_START to $NOW from host '`hostname`'" >> $OUT
else
   echo "processed entire log to $NOW from host '`hostname`'" >> $OUT
fi

echo "
Output fields are: " >>$OUT

#prints stripped bugids, each id in a single line
if [ "$ID" ] ; then
	echo "BUGIDS sorted in ascending order
	" >> $OUT
	awk '{
	i=1
	while (i <= NF) {
	{ if ($i ~ /[0-9][0-9][0-9][0-9][0-9][0-9][0-9]/)
	print $i }
	i++
	}}' $TMPFILE | sed "s/.*\($bugids\).*/\1/g"| sort |uniq >> $OUT
fi

#
#sorted by login, bugid
#

if [ "$LOGIN_ID" ] ; then
   echo "LOGIN BUGIDS sorted by LOGIN
   " >> $OUT
   sort +3 -0 $TMPFILE | awk '{
   { if ( NF > 3)
   printf ("%-12s", $3 )
   i=4
   while (i <= NF){
   { printf ("\t%s", $i) }
   i++}
   printf ("\n") } }' |\
   sed "s/[	][^[0-9]*\($bugids\)[^	]*/ #\1/g" |grep "[0-9]"|\
   sort | uniq >> $OUT
fi

#
#sorted by source, date, login
#

if [ "$SRC_DATE" ] ; then
   echo "SOURCE DATE LOGIN BUGIDS, sorted by SOURCE, DATE, LOGIN
      " >> $OUT
   sort +0 -3 $TMPFILE | awk '{printf ("%s \n", $1 ) 
   printf ("\t %s " " %s", $2, $3 ) 
   i=4
   while (i <= NF){
   { printf ("\t%s", $i) }
     i++}
     printf ("\n") }'|\
     sed -e "s/[	][^[0-9]*\($bugids\)[^	]*/ #\1/g" \
     -e '/^[ 	]*$/d'>> $OUT
fi

#
#sort by date, source, login
#

if [ "$DATE_SRC" ] ; then
   	echo "DATE SOURCE LOGIN BUGIDS, sorted by DATE, SOURCE
	" >> $OUT
	sort +1 $TMPFILE | awk '{printf ("%s \n", $2 )
	printf ("\t %s \n", $1 )
	i=3
	while (i <= NF){
	{ printf ("\t%s ", $i) }
	  i++}
	printf ("\n") }' |\
	sed -e "s/[	][^[ 0-9]*\($bugids\)[^	]*/ #\1/g" \
	-e '/^[    ]*$/d'>> $OUT
fi

rm -f /tmp/*$$
rm -f /tmp/src$$.work
rm -f $TMPFILE

