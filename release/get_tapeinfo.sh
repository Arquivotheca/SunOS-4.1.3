#! /bin/sh
#
#%Z%%M% %I% %E% SMI
#
# The input for this utility is any xdr-format table of contents 
# created by /usr/src/sundist/bin/mktp.  The default location of
# the TOCs is /usr/src/sundist, but you may specify an alternate 
# location for use with feature tape or upgrade releases, where
# 'mktp' is used to generate the distribution media.
#
# Execution of this script depends on the existence of the
# suninstall utility 'xdrtoc' in /usr/release/bin.  If this
# program is not available in the release bin directory, the 
# script will search for it in /proto/usr/etc/install.
#
# 'get_tapeinfo' has the following functions:
#
#	o Transforms the xdr binary into an ascii file to determine 
# 	  tape content details such as:
#	  file format, position, and size of each software set
#	  It is very useful to keep a copy of this output to maintain
#	  consistency between build cycles and domestic vs. export releases.
#
#	o Calculates contents for each volume of distribution media
#	  in terms of KB and MB, and totals them up for each tape set in MB.
#	  **This info is required by Project Coordinators for ECO.**
#
# Please send bugs/RFE's to Janet Timbol (janet@firenze) WSD Release Eng'g

here=`pwd`
sundist=/usr/src/sundist

#NOTE: Missing xdrtoc executable is a bug against 4.1.  Till that
#becomes available, use Janet's copies from 4.0.3

#xdrbin=/usr/release/bin/xdrtoc
xdrbin=/usr/krypton/janet/xdrtoc_`arch`

get_yn='(while read yn
do
        case "$yn" in
        [Yy]* ) exit 0;;
        [Nn]* ) exit 1;;
                * ) echo -n "Please answer y or n:  ";;
esac
done)'

echo -n "This utility can be used to do the following:

	o Generate ascii file equivalent of xdrtoc  
	o Calculate tape usage in KB by volume

Press <RETURN> to continue:  "
read return	

#Use default location in /usr/release/bin; if not there, look in /proto

[ ! -x $xdrbin ] && xdrbin=/proto/usr/etc/install/xdrtoc
[ ! -x $xdrbin ] && echo "'xdrtoc' executable not found!"  && exit 1

while :
do
	bad=; default=
	if [ -d $sundist ]; then
		echo -n "Use xdrtoc in $sundist?  (y/n) "
		eval "$get_yn" && default=1 
	fi
	if [ ! -d $sundist -o ! "$default" ]; then
		echo -n "Enter directory containing xdrtoc:  "
		read sundist
		[ ! -d $sundist ] && bad=1
	fi
	[ ! "$bad" ] && break
	echo -n "$sundist does not exist!  Quit? (y/n)  "
	eval "$get_yn" && exit 1 || sundist=/usr/src/sundist
done

cd $sundist
xdrfile=`ls sun*_*.xdr`

ascii_toc=; get_vol=
for i in $xdrfile; do
	ftype=`file $i | awk '{print $2}'`
	[ ! "$ftype" = "data" ] && \
	echo "Skipping $i; wrong format." && continue
	cat $i | $xdrbin > /tmp/$i
	echo "ASCII TOC is: /tmp/$i"
	ascii_toc="$ascii_toc /tmp/$i"
done
cd $here

for i in $ascii_toc; do
	columns=`grep Size $i| awk '{print NF}'` 
	[ "$columns" ] && [ $columns = 5 ] && get_vol="$get_vol $i" && \
	continue
	echo "Skipping '$i' (wrong format)."
done

hilite='=========================='
for i in $get_vol; do
	out=$i.vol
	echo $hilite > $out
	echo "$i" >> $out
	echo $hilite >> $out

	awk -Fof 'NR==1 {printf("%s \n\n", $1)}' $i >> $out
	awk '{if (NF==5 && $4 ~ /^[0-9]+$/)
		vol[$1] += $4
		total += $4
		}
	END {for (num in vol)
	printf ("\tVolume %d has %7d KBbytes \t %7.2f Megabytes\n\n", \
	num, vol[num]/1024, vol[num]/1048576)
	printf ("Total: \t\t\t\t\t %7.2f Megabytes (Approx)\n", total/1048576)
	} ' $i|sort >> $out
	echo "TAPE USAGE output: $out"
done
