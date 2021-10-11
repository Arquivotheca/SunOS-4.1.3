#! /bin/sh 
# %Z%%M% %I% %E% SMI;
:
for i in 1 2 3 4 5 6 7 8 ; do

	if [ -s tmp/titles.$i ] ; then
		rm -f tmp/titles.$i
#	else
#		echo "No tmp/titles.$i! I'm gone."
#		exit
# XXX Why should this be an error?
	fi

	if [ ! -s tmp/files.print.$i ] ; then
		echo "No tmp/files.print.$i! I'm gone."
		exit
	fi

	cd man$i

	for j in `cat ../tmp/files.print.$i` ; do

		/usr/5bin/echo "\n***** $j *****" >> ../tmp/titles.$i
		egrep '^\.I[^XP].*[A-Z]|^[^.].*fI[A-Z].* .*fP|^[A-Z][^.]*[A-Z]' $j | \
		egrep -v 'System V|Sun-|Sun386|[[]|' >> ../tmp/titles.$i
	done
	cd ..
done
#touch titles
# XXX what?

