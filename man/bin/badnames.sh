#! /bin/sh
# %Z%%M% %I% %E% SMI;
:
#  badnames: script to run from /usr/src/man;
#            no arguments are supported
#
#  badnames creates the badnames.[1-8] files
#            and touches /usr/src/man/badnames (XXX ?)
#
#  Dependencies:  the files tmp/man[1-8].badnames (XXX ?)
#            and tmp/files.print.[1-8] must exist
#            or badnames quits

for i in 1 2 3 4 5 6 7 8 ; do

	if [ -s tmp/badnames.$i ] ; then
		rm -f tmp/badnames.$i
#	else
#		echo "No tmp/badnames.$i! I'm gone."
#		exit
# XXX Why should this be an error?
	fi

	if [ ! -s tmp/files.print.$i ] ; then
		echo "No tmp/files.print.$i! I'm gone."
		exit
	fi

	cd man$i

	for j in `cat ../tmp/files.print.$i` ; do
		awk '/^\.SH.*NAME/,/^\.SH [^N]/ { 
			if ($1 !~ /^\.SH/) {
				print $0;
				k++
			} 
		}
		END { if (k > 3) {print "line breaks"}
			}' $j | egrep '\\[fsnc|^]|^\.|line breaks|bad dash' >> ../tmp/badnames.$i 
	done
	cd ..
done
#touch badnames
# XXX what?
