#! /bin/sh
# %Z%%M% %I% %E% SMI;
:
for i in 1 2 3 4 5 6 7 8 ; do

	if [ -s tmp/badpaths.$i ] ; then
		rm -f tmp/badpaths.$i
#	else
#		echo "No tmp/badpaths.$i! I'm gone."
#		exit
# XXX Why should this be an error?
	fi

	if [ ! -s tmp/files.print.$i ] ; then
		echo "No tmp/files.print.$i! I'm gone."
		exit
	fi

# the following section was "bin/hier:" in Prepfile
# tmp/hier built on xenon; not complete
# this section is commented out until it can be
# properly tested on a release machine

#	cd tmp
#	cd /proto
#	find . -name '*' -print | \
#	sed s/^\.// > /usr/src/man/tmp/hier
	cd man$i

# the following section was "badpaths: pagelist bin/hier"
# in Prepfile

	for j in `cat ../tmp/files.print.$i` ; do
		/usr/5bin/echo "\n*** $j ***" >> ../tmp/badpaths.$i 
		grep / $j | \
		tr ' ' '\012' | \
		grep '^/' | \
		sed -e 's/[^a-zA-Z0-9]*$//' -e 's/\\fI.*//' | \
		sort -u | \
		comm -23 - ../tmp/hier >> ../tmp/badpaths.$i 
# ../bin/hier is a dependency
	done
	cd ..
done
