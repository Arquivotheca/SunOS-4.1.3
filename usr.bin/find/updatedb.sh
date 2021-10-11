#! /bin/sh
# %Z%%M% %I% %E% SMI from UCB 4.6 85/04/22
# build "fast find" database -- normally run from crontab

UFSPATHS=			# directories to be put in the database
EXCLUDE=			# directories to exclude
NFSPATHS=			# NFS directories
NFSUSER=daemon			# userid for NFS find
LIBDIR=/usr/lib/find		# for subprograms
FINDHONCHO=root			# for error messages
FCODES=$LIBDIR/find.codes 	# the database

if [ $# -eq 0 ] ; then
	UFSPATHS="/"
	EXCLUDE='^/tmp|^/dev|^/usr/tmp|^/var/tmp'
	NFSPATHS=
elif set -- `getopt n:o:x: $*` ; then
	for arg
	do
		case $arg in
		-n) NFSPATHS="$NFSPATHS $2" shift 2 ;;
		-o) FCODES="$2" shift 2 ;;
		-x) EXCLUDE="$EXCLUDE${EXCLUDE:+'|'}$2" shift 2 ;;
		--) shift; break ;;
		esac
	done
	UFSPATHS="$@"
else
	echo -n "usage: `basename $0` "
	echo "[-n nfs-find-spec] [-o outfile] [-x exclude-regexp] find-spec ..."
	exit 1
fi

bigrams=/tmp/f.bigrams$$
filelist=/tmp/f.list$$
errs=/tmp/f.errs$$

trap 'rm -f $bigrams $filelist $errs; exit 1' 1 2 15

# Make a file list and compute common bigrams.
# Alphabetize '/' before any other char with 'tr'.
# If the system is very short of sort space, 'bigram' can be made
# smarter to accumulate common bigrams directly without sorting
# ('awk', with its associative memory capacity, can do this in several
# lines, but is too slow, and runs out of string space on small machines).

{
	if [ -n "$UFSPATHS" ] ; then
		find $UFSPATHS -fstype nfs -prune -o -print
	fi
	if [ -n "$NFSPATHS" ] ; then
		su $NFSUSER -c "find $NFSPATHS -print"
	fi
} |
	case "$EXCLUDE" in
	"") cat ;;
	*) egrep -v "$EXCLUDE" ;;
	esac |
	tr '/' '\001' |
	{ sort -f 2> $errs; } |
	tr '\001' '/' > $filelist

$LIBDIR/bigram < $filelist |
	{ sort 2>> $errs; } | uniq -c | sort -nr |
	awk '{ if (NR <= 128) print $2 }' | tr -d '\012' > $bigrams

if [ -s $errs ] ; then
	cat $errs
	stat=1
else
	# code the file list
	$LIBDIR/code $bigrams < $filelist > $FCODES
	stat=$?
	chmod 644 $FCODES
fi

rm -f $bigrams $filelist $errs
exit $stat
