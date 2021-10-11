#! /bin/sh
# %Z%%M% %I% %E% SMI;

echo "archiving: $*" | fmt
echo -n "OK? "
read ok
case $ok in
y*|Y*) : ;;
*) exit  ;;
esac

sccs edit -s $*
sccs delta -s $*
for i in $*
do
	mv SCCS/s.$i /usr/src/man/arch/SCCS
	rm -f $i
done
