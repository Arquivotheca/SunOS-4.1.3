#! /bin/sh
# %Z%%M% %I% %E% SMI;
:
for i in 1 2 3 4 5 6 7 8 ; do

	if [ -s tmp/badtroff.$i ] ; then
		rm -f tmp/badtroff.$i
#	else
#		echo "No tmp/badtroff.$i! I'm gone."
#		exit
# XXX Why should this be an error?
	fi

	if [ ! -s tmp/files.print.$i ] ; then
		echo "No tmp/files.print.$i! I'm gone."
		exit
	fi

	cd man$i

	for j in `cat ../tmp/files.print.$i` ; do

		echo "$j:  " >> ../tmp/badtroff.$i
		egrep '\\\\|^\.nr|^\.ds|^\.so|^\.tr|^\.cs|^\.na|^\.ad|^\.bp|^\.de|^\.ig' $j >> ../tmp/badtroff.$i

		tail -1 $j | awk '/^\.[ILT]P/ {
			print $0 " on last line"
		}' >> ../tmp/badtroff.$i 
	done
	cd ..
done
#touch badtroff
# XXX what?
