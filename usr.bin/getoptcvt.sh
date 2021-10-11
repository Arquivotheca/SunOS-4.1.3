#! /bin/sh
#
#	%Z%%M% %I% %E% SMI; from S5R3 1.3
#
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# This program changes all occurences of the SVR2 getopt invocation line
# to use the SVR3 version of getopt.
# Sedfunc is used to handle arguments with single quotes.
# If -b option is given, getoptcvt will create script that will usually work
# in releases previous to 3.0.
bflag=
while getopts b c
do
	case $c in
	b)  bflag=1;;
	\?) echo "getoptcvt [-b] file"
	    exit 2;;
	esac
done
shift `expr $OPTIND - 1`
if [ "$bflag" = 1 ]
then
	ed <<'!' - $1
1,$s/set[ 	][ 	]*--[ 	][ 	]*`getopt[ 	][ 	]*\(.*\)[ 	][ 	]*.*`/{\
if [ "$OPTIND" != 1 ]\
then\
	set -- `getopt \1 $*`\
else\
sedfunc() \
{\
echo "$1" | sed "s\/'\/'\\\\\\\\''\/g"\
}\
exitcode_=0\
while getopts \1 c_\
do\
	case $c_ in\
	\\?)\
		exitcode_=1\
		break;;\
	*)	if [ "$OPTARG" ]\
		then\
			optarg_=`sedfunc "$OPTARG"`\
			arg_="$arg_ '-$c_' '$optarg_'"\
		else\
			arg_="$arg_ '-$c_'"\
		fi;;\
	esac\
done\
shift `expr $OPTIND - 1`\
arg_="$arg_ '--'"\
for i_ in "$@"\
do\
	optarg_=`sedfunc "$i_"`\
	arg_="$arg_ '$optarg_'"\
done\
eval set -- "$arg_"\
test  $exitcode_ = 0\
fi ;}/
1,$p
Q
!
else
	ed <<'!' - $1
1,$s/set[ 	][ 	]*--[ 	][ 	]*`getopt[ 	][ 	]*\(.*\)[ 	][ 	]*.*`/{\
sedfunc()\
{\
echo "$1" | sed "s\/'\/'\\\\\\\\''\/g"\
}\
exitcode_=0\
while getopts \1 c_\
do\
	case $c_ in\
	\\?)\
		exitcode_=1\
		break;;\
	*)	if [ "$OPTARG" ]\
		then\
			optarg_=`sedfunc "$OPTARG"`\
			arg_="$arg_ -$c_ '$optarg_'"\
		else\
			arg_="$arg_ -$c_"\
		fi;;\
	esac\
done\
shift `expr $OPTIND - 1`\
arg_="$arg_ --"\
for i_ in "$@"\
do\
	optarg_=`sedfunc "$i_"`\
	arg_="$arg_ '$optarg_'"\
done\
eval set -- "$arg_"\
test  $exitcode_ = 0 ;}/
1,$p
Q
!
fi
