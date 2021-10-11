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

arch=

infile=$1 shift
outfile=$1
name=`basename $1` shift

for arg in "$name" "$@"
do
	case $arg in
	-sun*) arch=$arg ;;
	-*) opts="$opts $arg" ;;
	*) opts="$opts -D$arg=__${arg}__" ;;
	esac
done

case $arch in
"") arch=`arch -k` ;;
*) arch=`expr substr $arch 2 999` ;;
esac

# #keyword -> cpp directive
# #[whitespace] -> output
# %# -> #
# blank lines -> deleted
# = -> blank line

sed 's/^#/@#/;s/^@\(#[a-z]\)/\1/;s/^+//' $infile |
/lib/cpp -P -undef \
	"-D_NAME_=$name" "-D_QNAME_=\"$name\"" \
	"-D_ARCH_=$arch" "-D_QARCH_=\"$arch\"" "-D$arch=__${arch}__" \
	$opts |
sed -e '/^[ 	]*$/d;/^@$/d;s/^[%@=]//;s/__//g' > $outfile
