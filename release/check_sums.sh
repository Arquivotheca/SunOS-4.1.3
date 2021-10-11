:
#@(#)check_sums.sh 1.1 92/07/30 SMI
#Recursively get checksum for files under a directory.
#Remarks:
#    o	Output file is sorted by filenames;  it facilitates use of
#	'diff' to compare 'check_sums' output for different
#	directory hierarchies.
#
#    o  If current directory is not the target, you will be
#	prompted to supply complete path of another directory 
#	to be processed.
# 
#Suggested applications:
#    o Compare master tapes received from Tape Duplication
#
#    o Keep records of proto directories, especially for unbundled/feature 
#      tape builds, in order to track binaries that change through 
#      alpha-beta-FCS stages
#
#Please send bugs/RFE's to Janet Timbol (janet@firenze)
#

GET_YN='(while read yn
do
    case "$yn" in
    [Yy]* ) exit 0;;
    [Nn]* ) exit 1;;
        * ) echo -n "Please answer y or n:  ";;
esac
done)'
PWD=`pwd`
OUT=/tmp/check_sums.$$

echo -n "DIR=$PWD? (y/n)   "
eval "$GET_YN"
if [ "$?" = 1 ]; then
	while echo -n "Enter complete path of target directory:  "
	do
		read DIR
		[ ! -d $DIR ] && echo "$DIR not found." && continue
		cd $DIR; break
	done
fi
	
echo "Scratch file is $OUT."
echo "#`hostname`:  $PWD" > $OUT
echo "#`date`" >> $OUT
find . -type f -print > /tmp/$$
for i in `cat /tmp/$$`
do
	echo -n "`sum $i`	" >> $OUT
	echo "$i" >> $OUT
done
rm /tmp/$$
echo "" >> $OUT
sort +2 $OUT > $OUT.sort
echo "OUTPUT: $OUT.sort"
rm -f $OUT
cd $PWD
