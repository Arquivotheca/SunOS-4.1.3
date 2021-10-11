#! /bin/sh
# %Z%%M% %I% %E% SMI;

srcdir=/usr/src/man

/bin/rm /usr/src/man/tmp/invaders.[1-8]
/bin/rm /usr/src/man/tmp/Invaders

touch /usr/src/man/tmp/invaders.[1-8]

for i in 1 2 3 4 5 6 7 8
do
	cd $srcdir/man$i/SCCS
	find . -ctime -1 -a \! \( -user kennan -o -user gwenl \) \
		-exec echo {} >> $srcdir/tmp/invaders.$i
done

cat $srcdir/tmp/invaders.[1-8] > $srcdir/tmp/Invaders
if [ -s $srcdir/tmp/Invaders ]
then
	mail -s "Invaders" jah@caps < $srcdir/tmp/Invaders
fi
