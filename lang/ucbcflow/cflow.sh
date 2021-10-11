#! /bin/sh -
#
#	%Z%%M% %I% %E% SMI; from S5R3 1.9
#
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

PATH=/bin:/usr/bin
INVFLG=
DFLAG=
IFLAG=
DIR=/usr/lib
CC=cc			# $PATH has already been set
LINT1=/usr/lib/lint/lint1
TMP=/usr/tmp/cf.$$
TMPG=$TMP.g
trap "rm -f $TMP.?; kill $$" 1 2 3
echo "" >$TMP.g
while [ "$1" != "" ]
do
	case "$1" in
	-r)
		INVFLG=1
		;;
	-d*)
		DFLAG=$1
		;;
	-i*)
		IFLAG="$IFLAG $1"
		;;
	-f)
		cat $2 </dev/null >>$TMPG
		shift
		;;
	-g)
		TMPG=$2
		if [ "$TMPG" = "" ]
		then
			TMPG=$TMP.g
		fi
		shift
		;;
	-[IDU]*)
		o="$o $1"
		;;
	*.y)
		yacc $1
		sed -e "/^# line/d" y.tab.c > $1.c
		$CC -E $o $1.c | $LINT1 -H$TMP.j 2>/dev/null $1.c\
			| $DIR/lpfx $IFLAG >>$TMPG
		rm y.tab.c $1.c
		;;
	*.l)
		lex $1
		sed -e "/^# line/d" lex.yy.c > $1.c
		$CC -E $o $1.c | $LINT1 -H$TMP.j 2>/dev/null $1.c\
			| $DIR/lpfx $IFLAG >>$TMPG
		rm lex.yy.c $1.c
		;;
	*.c)
		$CC -E $o $1 | $LINT1 -H$TMP.j 2>/dev/null $1\
			| $DIR/lpfx $IFLAG >>$TMPG
		;;
	*.i)
		name=`basename $1 .c`
		$LINT1 -H$TMP.j 2>/dev/null <$1 | $DIR/lpfx >>$TMPG $name.c
		;;
	*.s)
		a=`basename $1 .s`
		as -o $TMP.o $1
		nm -ng $TMP.o | $DIR/nmf $a ${a}.s >>$TMPG
		;;
	*.o)
		a=`basename $1 .o`
		nm -ng $1 | $DIR/nmf $a ${a}.o >>$TMPG
		;;
	*)
		echo $1 "-- cflow can't process - file skipped" 1>&2
		;;
	esac
	shift
done
if [ "$INVFLG" != "" ]
then
	grep "=" $TMPG >$TMP.q
	grep ":" $TMPG | $DIR/flip >>$TMP.q
	sort <$TMP.q >$TMPG
	rm $TMP.q
fi
$DIR/dag $DFLAG <$TMPG
rm -f $TMP.?
