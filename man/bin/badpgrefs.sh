#! /bin/sh
# %Z%%M% %I% %E% SMI;

trap 'rm -f /tmp/fp.sorted$$' 0 9 15

:

sort -u tmp/files.print > /tmp/fp.sorted$$

for i in 1 2 3 4 5 6 7 8 ; do

	if [ -s tmp/badpgrefs.$i ] ; then
		rm -f tmp/badpgrefs.$i
#	else
#		echo "No tmp/badpgrefs.$i! I'm gone."
#		exit
# XXX Why should this be an error?
	fi

	if [ ! -s tmp/files.print.$i ] ; then
		echo "No tmp/files.print.$i! I'm gone."
		exit
	fi

	cd man$i

	for j in `cat ../tmp/files.print.$i` ; do
		/usr/5bin/echo "\n*** $j ***" >> ../tmp/badpgrefs.$i
		grep '^\.BR.*([1-8]' $j | \
		sed -e 's/\.BR *//' -e 's/  *//g' -e 's/).*$//' \
		-e 's/(/./' -f /usr/src/man/bin/badpgrefs.sed | \
		sort -u | \
		comm -23 - /tmp/fp.sorted$$ >> ../tmp/badpgrefs.$i
	done
	cd ..
done

rm -f /tmp/fp.sorted$$

#touch badpgrefs
# XXX what?
