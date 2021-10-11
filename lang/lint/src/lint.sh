#! /bin/sh -
#
#	%Z%%M% %I% %E% SMI; from S5R2 1.5
#
# New lint shell script.  Changed to make lint(1) act as much as is possible
# like a different version of the cc(1) command.  This includes the notion of
# a ``lint .o'' (.ln) and incremental linting.  Thu Jan 27 10:07:15 EST 1983
#
TOUT=/usr/tmp/tlint.$$		# combined input for second pass
HOUT=/usr/tmp/hlint.$$		# header messages file
LDIR=/usr/lib/lint		# where first & second pass are
LLDIR=/usr/5lib/lint		# where lint libraries are found
PATH=/usr/5bin:/usr/bin
CCF="-E -C -Dlint"		# options for the cc command
LINTF=				# options for the lint passes
FILES=				# the *.c and *.ln files in order
NDOTC=				# how many *.c were there
DEFL=$LLDIR/llib-lc.ln		# the default library to use
LLIB=				# lint library file to create
CONLY=				# set for ``compile only''
pre=				# these three variables used for
post=				# handling options with arguments
optarg=				# list variable to add argument to
#
trap "rm -f $TOUT $HOUT; exit 2" 1 2 3 15
#
# First, run through all of the arguments, building lists
#
#	lint's options are "abchl:no:puvxz"
#	cc/cpp options are "I:D:U:gO"
#
for OPT in "$@"
do
	if [ "$optarg" ]
	then
		if [ "$optarg" = "LLIB" ]	# special case...
		then
			OPT=`basename $OPT`
		fi
		eval "$optarg=\"\$$optarg \$pre\$OPT\$post\""
		pre=
		post=
		optarg=
		continue
	fi
	case "$OPT" in
	*.c)	FILES="$FILES $OPT"	NDOTC="x$NDOTC";;
	*.ln)	FILES="$FILES $OPT";;
	-*)	OPT=`echo $OPT | sed s/-//p`
		while [ "$OPT" ]
		do
			O=`echo $OPT | sed 's/\(.\).*/\1/p'`
			OPT=`echo $OPT | sed s/.//p`
			case $O in
			p)	LINTF="$LINTF -p"
				CCF="$CCF -Qoption cpp -T"
				DEFL=$LLDIR/llib-port.ln;;
			n)	LINTF="$LINTF -n"
				DEFL=;;
			c)	CONLY=1;;
			[abhquvxz]) LINTF="$LINTF -$O";;
			[gO])	CCF="$CCF -$O";;
			[IDU])	if [ "$OPT" ]
				then
					CCF="$CCF -$O$OPT"
				else
					optarg=CCF
					pre=-$O
				fi
				break;;
			l)	if [ "$OPT" ]
				then
					FILES="$FILES $LLDIR/llib-l$OPT.ln"
				else
					optarg=FILES
					pre=$LLDIR/llib-l
					post=.ln
				fi
				break;;
			o)	if [ "$OPT" ]
				then
					OPT=`basename $OPT`
					LLIB="llib-l$OPT.ln"
				else
					LLIB=
					optarg=LLIB
					pre=llib-l
					post=.ln
				fi
				break;;
			*)	echo "lint: bad option ignored: $O";;
			esac
		done;;
	*)	echo "lint: file with unknown suffix ignored: $OPT";;
	esac
done
#
# Second, walk through the FILES list, running all .c's through
# lint's first pass, and just adding all .ln's to the running result
#
if [ "$NDOTC" != "x" ]	# note how many *.c's there were
then
	NDOTC=1
else
	NDOTC=
fi
if [ "$CONLY" ]		# run lint1 on *.c's only producing *.ln's
then
	for i in $FILES
	do
		case $i in
		*.c)	T=`basename $i .c`.ln
			if [ "$NDOTC" ]
			then
				echo $i:
			fi
			(cc $CCF $i | $LDIR/lint1 $LINTF -H$HOUT $i >$T)
			$LDIR/lint2 -H$HOUT
			rm -f $HOUT;;
		esac
	done
else			# send all *.c's through lint1 run all through lint2
	rm -f $TOUT $HOUT
	for i in $FILES
	do
		case $i in
		*.ln)	cat <$i >>$TOUT;;
		*.c)	if [ "$NDOTC" ]
			then
				echo $i:
			fi
			(cc $CCF $i|$LDIR/lint1 $LINTF -H$HOUT $i >>$TOUT);;
		esac
	done
	if [ "$LLIB" ]
	then
		cp $TOUT $LLIB
	fi
	if [ "$DEFL" ]
	then
		cat <$DEFL >>$TOUT
	fi
	if [ -s "$HOUT" ]
	then
		$LDIR/lint2 -T$TOUT -H$HOUT $LINTF
	else
		$LDIR/lint2 -T$TOUT $LINTF
	fi
fi
rm -f $TOUT $HOUT

