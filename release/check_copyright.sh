#! /bin/sh
#
#%Z%%M% %I% %E% SMI
#
########################################################################
#
#This utility extracts the Copyright file from the following media 
#types for both sun3 and sun4 application architectures:
#
#	Half Inch	(mt) - Density = High (6250) or Low (1600) 
#	Quarter Inch	(st) - Density = High (QIC24) or Low (QIC11)
#
#Requirements:
#
#   o	You must login to the tapehost and execute this script.
#	If your tapehost is a release machine, this utility should
#	be accessible under /usr/release/bin.  If you do not
#	find it there, please contact the RE for that particular
#	machine.
#
#   o	The tapehost must be running 4.0.3 or later, to support
#	the 'asf' option of the 'mt' command. 
#
#   o	If you are verifying a SunOS release, the TOC conversion
#	utility named "xdrtoc_`arch`" must be available.
#	You do not have to worry about this if you already have
#	aprime:/usr/krypton already mounted on the tapehost 
#	(which is usually the case for our release machines).
#	If /usr/krypton is not accessible, make a copy in /tmp 
#	of your tapehost, using this command:
#
#	   rcp aprime:/usr/krypton/janet/xdrtoc_`arch` /tmp
#
#Usage: /usr/release/bin/check_copyright [-n]
#
#	By default, each tape is rewound after verification.
#	If you're in a rush and need to save time, suppress the
#	rewind at the end using the -n option. 
#
#	You will be asked for the following information:
#
#	Product Type to be verified  (see below)
#	Media to be verified (qtr/half)
#	Density of media to be verified (hi/lo)
#
#Product Types Supported:
#
#   You will be presented with a menu of products to be verified,
#
#   SunOS 
#	Choosing this item will extract the TOC from File#1
#	to determine the following info: copyright location, 
#	total number of volumes, and the volume number 
#	being processed.  Tape format is always 'boot'.
#
#   Crypt
#	Choosing this item will extract the copyright file
#	from File#12.  Total number of volumes is always 1,
#	tape format is always 'tar'.
#
#   Diag Exec
#	Choosing this item will extract the TOC from File#2
#	to determine the copyright location.  Total number of 
#	volumes is always 1, tape format is always 'tar'.
#
#   Other
#	If you choose this item, you will be prompted to specify 
#	a file number for the copyright location, or indicate 
#	that it is unknown by entering 'X'.  You will also
#	be prompted to enter the total number of volumes in the 
#	media set.
#
#Verification Process:
#
#	The script checks for the existence of certain copyright 
#	components, listed below.  However, it will also display 
#	the Copyright for your review, and save intermediate
#	files for closer scrutiny.  The copyright components
#	checked are:
#		Copyright year
#		Tape Size
#		Part Number
#		Rev.
#		Volume# of Total Volumes
#		Density
#
#Hint:
#
#	If the copyright file cannot be found where it is expected,
#	Choose Product Type = Other, Copyright location = X, and
#	let the script find the last file.
#
#Please send bugs/RFE's to Janet Timbol (janet@firenze)
#
########################################################################
#
#Screen yes/no reponses
#

usage="USAGE:  $0 [ -n ] "
if [ $# -gt 1 ] ; then
	echo "$usage" && exit 1
else
	case $1 in
		-n ) no_rewind=1;;
		'' ) ;;
		*  ) echo $usage && exit 1;;
	esac
fi

get_yn='(while read yn; do
	case "$yn" in
		[Yy]* ) exit 0;;
		[Nn]* ) exit 1;;
		     * ) echo -n "Please answer y or n:  ";;
	esac
done)'

tmp=/tmp
here=`pwd`
rawdev=/dev/nr

#
#Floppies not supported--awaiting sample media and more info
#

while echo -n "Enter media to be verified (qtr/half):   "; do
	read media
	case $media in
		[Qq]* ) media_type=quarter; sz=1/4; d_unit=QIC; break;;
		[Hh]* ) media_type=half; sz=1/2; d_unit=bpi; break;;
		    * ) echo "Try again.";;
	esac
done

echo -n "Is this a high-density tape ie, 6250 bpi or QIC-24?  (y/n):  "
eval "$get_yn"
	case $? in
		0 ) if [ "$media_type" = "quarter" ]; then
				device=${rawdev}st8
				dens=24; blksz=200b
			else
				device=${rawdev}mt8
				dens=6250; blksz=20b
			fi; break;;
		1 ) if [ "$media_type" = "quarter" ]; then
				device=${rawdev}st0
				dens=11; blksz=126b
			else
				device=${rawdev}mt0
				dens=1600; blksz=20b
			fi; break;;
	esac

#
#Assume only 'SunOs' and 'Other' product category have multiple vols
#
while echo -n "		
		PRODUCT TYPES

	1)	Full SunOS release (TOC is 2nd file)
	2)	Crypt Kit (No TOC, Copyright is 13th file)
	3)	Diag Exec release, (TOC is 3rd file)
	4)	Other (TOC &  Copyright location unknown)

Choose type of product being verified (1-4):  "; do
	read choice
	case $choice in
	1|[Ff]* ) toc_asf=1; convert_toc=1; multi_vol=1; prod="SunOS"
		  tp_format='BOOT';;
	2|[Cc]* ) copy_asf=12; prod="Crypt"; tp_format=TAR;;
	3|[Dd]* ) toc_asf=2; prod="DiagExec"; tp_format=TAR ;;
	4|[Oo]* ) copy_asf=last; prod="Other"; multi_vol=1;;
	      * ) echo "$choice is not a valid selection"
		  echo -n "Press <RETURN> to continue:  "
		  read return && /usr/ucb/clear && continue;;
	esac
	break
done

mydir=${tmp}/`whoami` 
workdir=${tmp}/${media_type}; voldir=${mydir}/${prod}_${media_type}
for i in $mydir $voldir $workdir; do
	[ ! -d $i ] && mkdir $i
done
cd $workdir; rm -f *

if [ "$prod" = "Other" ] ; then
	while : ; do
	   echo -n \
	   "Enter total number of volumes for this media ($media_type):  "
	   read vol_sum
	   case $vol_sum in
		[1-9] | [1-9][1-9] ) break;;
		* ) echo "Number should be between 1-99!";;
	   esac
	done
	[ "$vol_sum" -ne 1 ] && multi_vol=1 || multi_vol=
fi

if [ "$convert_toc" ]; then
	xdr_bindir=/usr/krypton/janet
	while [ ! -x $xdr_bindir/xdrtoc_`arch` ]; do
		echo "We need 'xdrtoc_`arch`' utility!"
		echo -n "Enter directory where it can be found:  "
		read xdr_bindir
	done
fi

bad=
cleanup='(cd $here; 
	rm -rf $workdir
	rm -f $tmp/*$$ 
	[ "$bad" ] && exit 1
	echo ""
	echo "Output and error files are in the directory $voldir."
	echo "You may want to clean it up when you are done, using:"
	echo "	rm -rf $voldir"
)'

chk_ok='( if egrep -vs " records " $errs ; then
	  	echo "Tape Access Operation failed!"
		bad=1; eval "$cleanup"
		exit 1
	  else
		exit 0
fi )'

#
#Check if 'asf' supported
#
os_release=`egrep '^SunOS Release ' /etc/motd | awk '{print $3}'`
case $os_release in
   4.0.3* | 4.1* ) skip=asf;;
   * ) echo "Please execute this utility on a system running 4.0.3 or later."
       bad=1; eval "$cleanup";	exit 1;;
esac

trap 'echo ABORTED!; bad=1; eval "$cleanup"; exit 1' 1 2 3 15
errs=$tmp/errs_$$; rm -f $errs

#
# Begin multiple volume loop
#

while :
do
   this_vol=	
   if [ ! "$prod" = "SunOS" ]; then
	if [ "$multi_vol" ] ; then
		while : ; do 
		   echo -n "Enter Volume Number being processed:  "
		   read this_vol
			case $vol_sum in
			   [1-9] | [1-9][1-9] ) break;; 
			   * ) echo "^GNumber should be between 1-99!";;
			esac
		done
	else
		this_vol=1
        fi
   fi
   if [ "$prod" = "Other" ]; then
	while echo -n \
"Enter last file number (first file=0) on this media, or 'x' if unknown:  "; do
	   read copy_asf
	   case "$copy_asf" in
		[0-9] | [0-9][0-9] ) break;;
		[Xx]* ) copy_asf=unknown; break;;
		    * ) echo "Answer should be 'x' or a number.";;
	   esac
	done
   fi

   /usr/ucb/clear
   echo "Rewinding tape..."
   mt -f $device rew
   if [ ! $? = 0 ]; then
	echo "Unable to access $device for $media_type media!"
	bad=1; eval "$cleanup"
	exit 1
   fi

   if [ "$toc_asf" ] ; then
	echo "Extracting toc..."
	mt -f $device $skip $toc_asf 2>> $errs
	eval "$chk_ok" || exit 1
	dd if=$device of=toc_dd bs=${blksz} 2>> $errs
	eval "$chk_ok" || exit 1
	if [ ! "$convert_toc" ]; then   #Must be Diag Exec
		mv toc_dd toc_read
		copy_asf=`wc -l toc_read| awk '{print $1}'`
	else
		cat toc_dd | $xdr_bindir/xdrtoc_`arch` > toc_read
		if [ ! "$?" = 0 ] ; then
			echo "Can't read TOC!" 
			bad=1; eval "$cleanup"; exit 1
		fi
		rm -f toc_dd
		this_vol=`egrep -i "^volume" toc_read | awk '{print $2}'`
		vol_sum=`tail -1 toc_read | awk '{print $1}'`
		echo "This is volume #${this_vol} of ${vol_sum}..."
		copy_asf=`egrep -i copyright toc_read | \
		egrep "[ 	]${this_vol}[ 	]"| awk '{print $2}'`
	fi
   fi

   copy_out=copyright
   echo "This is volume #${this_vol} of ${vol_sum}..."
   if [ ! "$copy_asf" = "unknown" ] ; then
	echo "Forwarding tape to Copyright file (#$copy_asf)"
	mt -f $device $skip $copy_asf 2>> $errs
	eval "$chk_ok" || exit 1
	dd if=$device of=$copy_out bs=${blksz} 2>> $errs
	eval "$chk_ok" || exit 1
   else
	echo "Searching for Copyright file..."
	count=1; copy_asf=
	while : ; do
		mt -f $device $skip $count 2> $errs
		eval "$chk_ok" || exit 1
		dd if=$device of=/dev/null bs=${blksz} 2> $errs
		if egrep -s '^0\+0 records in' $errs && \
		egrep -s '^0\+0 records out' $errs ; then
			echo \
			"Forwarding to last file (#$copy_asf)..."
			mt -f $device $skip $copy_asf 2>errs
			dd if=$device of=$copy_out bs=${blksz} 2>> $errs
			break
		else
			copy_asf=$count; count=`expr $count + 1`
		fi
	done
   fi

#
#Check to see if extracted file IS the copyright
#
   for i in reserve license; do
	egrep -is $i $copy_out && break
	echo "ERROR: Extracted file doesn't look like Copyright!"
	eval "$cleanup"; exit 1
   done

   this_year=`date | awk '{print $NF}'`
   egrep -i "^copyright " $copy_out | egrep -s $this_year

   if [ ! "$?" = 0 ] ; then
	echo "WARNING: Copyright period does not include $this_year" | \
	tee -a check_errs
   fi

   for i in $sz "Part Number " " Rev." "$this_vol of $vol_sum"
   do
	egrep -is "$i" $copy_out
	[ ! "$?" = 0 ]  && echo "WARNING:  '$i' string NOT FOUND!" | \
	tee -a check_errs
   done
   case $media_type in
	quarter) egrep $d_unit $copy_out | egrep "$dens" > $tmp/$$;;
	   half) egrep -i $d_unit $copy_out | egrep "$dens" > $tmp/$$;;
   esac	
   [ ! -s $tmp/$$ ] && \
   echo "WARNING: $d_unit/$dens Info Missing/Wrong!" | \
   tee -a check_errs
   if [ "$tp_format" ]; then
      egrep -i ' format' $copy_out | egrep -i "${tp_format}" > $tmp/$$
   else
      egrep -i ' format' $copy_out > $tmp/$$
   fi
   [ ! -s $tmp/$$ ] && echo "WARNING: Format Info Incorrect!" | \
   tee -a check_errs

   rm -f ${voldir}/*${this_vol}
   for i in `ls -F1 | egrep -v /$`; do
	[ "$i" = ${copy_out} ] && \
	mv $i ${voldir}/${copy_out}_asf${copy_asf}_vol${this_vol} && continue
	mv $i ${voldir}/${i}_vol${this_vol}
   done
   echo "
Please make sure this matches the tape label:
"
   echo "===========================Copyright File==========================="
   cat ${voldir}/${copy_out}_asf${copy_asf}_vol${this_vol}
   echo "====================================================================
   "
   if [ ! "$no_rewind" ] ; then
	echo "Rewinding tape..."
	mt -f $device rew
   fi
   if [ "$multi_vol" ] ; then
	echo -n \
	"Process another volume Media=$media_type, Product=$prod? (y/n)  "
	eval "$get_yn" || break
	echo -n "Load next volume for this media set, press <RETURN>  "
	read return
   else
	break
   fi

done

#
#End multiple volume loop
#
echo "DONE!"
eval "$cleanup"; exit 0

