#! /bin/sh
#
#	%Z%%M% %I% %E% SMI
#       mksrc [ -fs ] [ device ] [ srcdir ] intsrc|sunsrc|winsrc|fullsrc|sccs
#
PATH=`pwd`:$PATH; export PATH

intsrc_exclude="`pwd`/intsrc_exclude_list"
sunsrc_exclude="`pwd`/sunsrc_exclude_list"
winsrc="`pwd`/winsrc_list"
README="`pwd`/README"
SRC=/usr/src
tape=/dev/nrmt8

cmdname=`basename $0`

if [ `whoami` != root ]; then
        echo ${cmdname}: must be run as root
        exit 2
fi

set -- `getopt f:s: $*`
if [ $? != 0 ]; then
	echo $usage
	exit 2
fi
for i in $*; do
	case $i in
	-f)	tape=$2; shift 2;;
	-s)	SRC=$2; shift 2;;
	--)	shift; break;;
	esac
done

case $tape in
/dev/nrmt*)
	type=half
	format=half
	;;
/dev/nrst*)
	type=quarter
	format=half
	;;
/dev/rmt*)
	tape=`echo $tape | sed -e 's#/dev/r#/dev/nr#'`
	type=half
	format=half
	;;
/dev/rst*)
	tape=`echo $tape | sed -e 's#/dev/r#/dev/nr#'`
	type=quarter
	format=quarter
	;;
*)
	echo ${cmdname}: unknown tape device \"$tape\"
	exit 2
	;;
esac

if [ $# -lt 1 ]; then
	srctype=intsrc
	echo ${cmdname}: I assume you want \"$srctype\"
else
	srctype=$1
fi
case "$srctype" in
intsrc|fullsrc)
	tarfiles="."
	exclude_files=""
	excl_opt=""
	if [ $type = "quarter" ]; then
		format=compressed
	fi
	;;
sunsrc)
	tarfiles="."
	exclude_files=${sunsrc_exclude}
	excl_opt=X
	if [ $type = "quarter" ]; then
		format=compressed
	fi
	;;
winsrc)
	tarfiles="`cat ${winsrc}`"
	exclude_files=""
	excl_opt=""
	;;
sccs)
	tarfiles="SCCS_DIRECTORIES"
	exclude_files=""
	excl_opt=""
	format=compressed
	if [ $type = "quarter" ]; then
		echo ${cmdname}: invalid media type for \"$srctype\"
		exit 1
	fi
	;;
	
*)
	echo ${cmdname}: unknown source category \"$srctype\"
	exit 2
	;;
esac

part=`grep $srctype part_numbers | grep $type | awk '{print $3}'`
cat Copyright | sed -e "s/_PART_/$part/" -e "s/_TYPE_/$srctype/" > $README
cat README.$format >> $README
cat README.$srctype >> $README

echo "Load the ${type}-inch tape, then press RETURN"; read x
mt -f $tape rew || exit
echo "Transferring copyright..."
dd if=$README of=$tape conv=sync || exit
if [ "$format" = "compressed" ]; then
	echo "Transferring extract script..."
	tar cf $tape extract README || exit
	echo "Transferring source files..."
	cd $SRC
	tar cf$excl_opt - $exclude_files $tarfiles | \
	    compress -fc | tapeblock $tape 10240 || exit
else
	echo "Transferring source files..."
	cd $SRC
	tar cf$excl_opt $tape $exclude_files $tarfiles || exit
fi
echo "Transferring copyright..."
dd if=$README of=$tape conv=sync || exit
mt -f $tape offline || exit
echo "Done with ${type}-inch tapes."
