#! /bin/sh
#
# %Z%%M% %I% %E% SMI
#

usage() {
echo "usage: `basename $0` master-file config-name [option...]"
exit 1
}

case $# in
0|1) usage ;;
esac

file=$1 shift
name=$1

for arg
do
	case $arg in
	-*) opts="$opts $arg" shift ;;
	*) opts="$opts -D$arg=__${arg}__" shift ;;
	esac
done

case $name in
*[0-9]*)	qname=\"$name\" ;;
*)		qname=$name ;;
esac

sed '/%#/d;s/^/@/;s/^@%/#/' $file |
/lib/cpp -P -undef "-D_NAME_=$name" "-D_QNAME_=$qname" $opts |
sed -e '/^$/d;s/^@//;s/__//g' > $name
