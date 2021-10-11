#!/bin/csh -f
#
#	%Z%%M% %I% %E% SMI; from UCB 4.6 85/04/22
#
set SRCHPATHS = "/"			# directories to be put in the database
set LIBDIR = /usr/lib/find		# for subprograms
set FINDHONCHO = root			# for error messages
set FCODES = /usr/lib/find/find.codes	# the database 

set path = ( $LIBDIR /usr/ucb /bin /usr/bin )
set bigrams = /tmp/f.bigrams$$
set filelist = /tmp/f.list$$
set errs = /tmp/f.errs$$

# Make a file list and compute common bigrams.
# Alphabetize '/' before any other char with 'tr'.
# If the system is very short of sort space, 'bigram' can be made
# smarter to accumulate common bigrams directly without sorting
# ('awk', with its associative memory capacity, can do this in several
# lines, but is too slow, and runs out of string space on small machines).

nice +10
find ${SRCHPATHS} -fstype nfs -prune -o -print | tr '/' '\001' | \
   (sort -f; echo $status > $errs) | \
   tr '\001' '/' >$filelist
$LIBDIR/bigram <$filelist | \
   (sort; echo $status >> $errs) | uniq -c | sort -nr | \
   awk '{ if (NR <= 128) print $2 }' | tr -d '\012' > $bigrams

# code the file list

if { grep -s -v 0 $errs } then
	echo 'squeeze error: out of sort space' | mail $FINDHONCHO
else
	$LIBDIR/code $bigrams < $filelist > $FCODES
	chmod 644 $FCODES
endif
rm -f $bigrams $filelist $errs
