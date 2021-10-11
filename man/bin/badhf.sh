#! /bin/sh
# %Z%%M% %I% %E% SMI;

:
#cp tmp/man2.badhf tmp/man1.badhf
# XXX what?
for i in 1 2 3 4 5 6 7 8 ; do

	if [ -s tmp/badhf.$i ] ; then
		rm -f tmp/badhf.$i
#	else
#		echo "No tmp/badhf.$i! I'm gone."
#		exit
# XXX Why should this be an error?
	fi

	if [ ! -s tmp/files.print.$i ] ; then
		echo "No tmp/files.print.$i! I'm gone."
		exit
	fi

	cd man$i

	for j in `cat ../tmp/files.print.$i` ; do

		awk 'BEGIN {FS="\""} /^\.TH/ {
			F1 = split($1,F," ")
			F2 = split($2,F," ")
			F3 = split($3,F," ")
			if ( NF != 3 || F1 != 3 || F2 != 3 || F3 != 0 ) {
				printf "%s:  %s\n", FILENAME, $0
			}
		}' $j >> ../tmp/badhf.$i
		grep -n '\.nr.*]W' $j >> ../tmp/badhf.$i

	done
	cd ..
done
#touch badhf
# XXX what?
