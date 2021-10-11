#! /bin/sh
# @(#)getNAME.sh 1.15 88/10/24 SMI;
PATH=/usr/ucb:/bin:/usr/bin
IFS=' 	'

# getNAME -- a shell implementation of /usr/lib/getNAME that
# is more robust that an earlier C program by the same name.
# This script breaks out the NAME section of a man page and
# formats the output in several ways: for use with whatis,
# apropos, for intro man pages, and raw NAME lines for input
# to various filters.

# Break out options into args list.
set -- `getopt -itapmrsS: $*`
opt=ptx
filter=fmt
troff="troff -man -a"

# Parse args list until all options are read.  In a conflict,
# leftmost arg wins.  Args remaining after the last flag option
# are taken as filenames.
while test $# -gt 0
do
	case $1 in
	--) # Skip spurious argument from getopt.
	;;
	-i)	# Unless an option is already set, set for intro format.
		if test "$opt" = "ptx" ; then opt=intro ; fi
	;;
	-t)	# Set for for TOC format.
		if test "$opt" = "ptx" ; then opt=toc ; fi
	;;
	-p)	# Change filenames of man pages to printed-format page refs. 
		print=1 # add font changes for printed intros
	;;
	-a)	# Alternate names for page.
		if test "$opt" = "ptx" ; then opt=alt ; fi
	;;
	-r)	# Produce the raw NAME line data in one long line per file,
		# or unformatted source of section given by -S.
		if test "$opt" = "ptx" # raw
		then
			opt=raw 
			filter='tail +3'
			troff=cat
			raw=1
		fi
	;;
	-s)	# Produce a table of alternate ".so" files by which man
		# is to be able to refer to a page.
		if test "$opt" = "ptx" ; then opt=mkso ; fi # table of .so's
	;;
	-S)	# Extract a named section (such as FILES).
		opt=sect ; shift ; sect="$1"
	;;
	*) break
	;;
	esac ; shift
done

# Old getNAME dropped core if there were no filename args.
# Neither the C program nor this script handle input from pipes.
case $# in
0)	echo "getNAME: no filename arguments" ; exit 1
;;
esac

# Get NAME lines.
for i in $*
do # yell about a missing file, but continue with next arg.
if test -f $i
then 

	# handle "no section-number suffix" cases
	case $i in
	*.*)	filename=`basename $i`
	;;
	*)	filename=`basename $i`.0
	;;
	esac

	# getNAME needs the .TH line, NAME section (line), and the
	# basename and suffix of the filename.  The C program only
	# accepted one line of BUFSIZE bytes.  This is inadequate for
	# building some 3X and 3N, and other .so files and whatis entries.
	# The backslashes in the sedscript for nameline requires
	# protection from two shells and sed.  The sequence of 8
	# backslashes ultimately reduces to one.  An awk script
	# might have been a better choice theoretically, but this works.
	thline=`grep "^\.TH" $i | sed 's/\(".*"\).*$/\1/'`
	nameline=`sed -n -e "/^\.SH[ 	]*"'"*'NAME'"*'"/,/^\.SH/p" $i \
		| sed -e "/^\./d" \
		-e "s/\\\\\\\\f.//g" \
		-e "s/\\\\\\\\s0//g" \
		-e "s/\\\\\\\\s[+-].//g" \
		-e "s/\\\\\\\\[cn]//g" \
		-e "s/\\\\\\\\t/	/g" \
		-e "s/\\\\\\\\(em/\\\\\\\\-/" \
		-e "s/ - / \\\\\\\\- /" \
		| tr '\012' ' ' ; echo ""`
	base=`echo $filename | sed "s/\..*$//"`
	suffix=`echo $filename | sed "s/^.*\.//"`
	mandir="man`echo $suffix | sed 's/[a-z]//'"
	if test $print
	then	# Alter suffix to "(3X)" form, add fonts to base
		# The y command for sed is a godawful thing.  No
		# [a-z] notation.
		suffix=`echo $suffix | sed -e "s/.*/(&)/" \
		-e "y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/"`
		base="\\fB${base}\\fR"
		fB='\fB'
		fR='\fR'
	else # Replace leading "." only.
		suffix=".$suffix"
	fi
	# Produce results on stdout.
	case $opt in	# display data as per option
	raw)	# Produce raw NAME line.
		echo $nameline
	;;
	toc)	# Produce the same stuff the old -t flag did.
		echo "${base}${suffix} $thline	$nameline"
	;;
	ptx)	# Produce the same stuff the as the old default.
		echo "$thline $nameline"
	;;
	intro)	# Produce an intro-page entry for each alternate name.
		# The only difference is the addition of the -p flag
		# font changes and parens.
		for j in $nameline
		do
			if test "$j" = "\-"
			then break
			else
				item="$fB`echo $j | sed 's/,//'`$fR"
				summary=`echo $nameline  \
				| sed -e "s/^.*\\\\\\\\-//" -e "s/^.*(em//"`
				echo "$item	${base}${suffix}	$summary"
			fi
		done
	;;
	alt)	# Produce list of alternate names.
		for j in $nameline
		do
			if test "$j" = "\-"
			then break
			else
				item=`echo $j | sed "s/,//"`
				if test "$item" != "$base"
				then 
					echo "${item}${suffix}	${base}${suffix}"
				fi
			fi
		done
	;;
	mkso)	# Produce a script to build .so files.
		for j in $nameline
		do
			if test "$j" = "\-"
			then break
			else
				item=`echo $j | sed "s/,//"`
				if test "$item" != "$base" -a ! -f SCCS/s.${item}${suffix}
				then 
					echo "echo .so $mandir/${base}${suffix} > ${item}${suffix}" " ; " 'echo ".\\\" ~%~Z~%~%~M~% ~%~I~% %~E~% SMI;" >> '"${item}${suffix} ; echo ${item}${suffix} 2>&1" | sed "s/~//g" ; 
				fi
			fi
		done
	;;
	sect)	# Get a section of a man page.
		case $raw in
		1)  sed -n -e "/^\.SH[ 	]*"'"*'$sect'"*'"/,/^\.SH/p" $i\
			| sed -e "/^\.SH/d"
		;;
		*) ( echo ".nh" ; \
			echo ".TH `basename $i`" ; \
			echo ".nf" ; \
			sed -n \
			-e "/^\.SH[ 	]*"'"*'$sect'"*'"/,/^\.SH/p" $i\
			| sed -e "/^\.SH/d" \
			-e '/^\.IP/ s/"//g' \
			-e '/^\.IP/ s/[ 	][0-9]*$//' \
			-e '/^\.IP/i\
' \
			-e 's/^\.IP/.sp 2/' \
			-e "s/^\.TP.*/.sp 2/" \
		) \
			| $troff | sed \
			-e "/^$sect/d" \
			-e "/^`basename $i`(/d" \
			-e "/Last change:/d" \
			-e "s/^ //" \
			| $filter
		;;
		esac
	;;
	esac
	shift

else echo "getNAME: $i: file not found" > /dev/tty
fi
done
