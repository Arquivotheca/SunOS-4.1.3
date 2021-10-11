PATH=../bin:$PATH
case $# in
0)	# display usage lines
	echo "pix [-t] [-d database] filename ..."
	echo "	-t		Test. (produce output on terminal)"
	echo "	-d database	Add words and page names to `database'."
;;
esac

database=/dev/null
set -- `getopt -td: $*`
while test $# -gt 0
do
	case $1 in
	--) 	# forget it.
	;;
	-t)		# test, direct output to standard output instead of building
			# ed-script files with the .IX suffix.
		testrun=1
	;;
	-d)
		shift ; database=$1
	;;
	*)	break
	;;
	esac
	shift
done

#	loop until all filename args are done
while test $# -gt 0
do
	# Note files searched in Pix.log.  Check for previous pix edits.
	# If there are any, note in Pix.log.

	if test ! -z "`grep -l 'output from pix' $1`"
	then echo "$1: has pix output" >> Pix.log
	else echo "$1" >> Pix.log
	fi

	# Get pieces of man page names.
	basename=`basename $1`
	base=`echo $basename | sed -e 's/\..*$//' -e 's/Intro/intro/'`
	file=$base
	suffix=`echo $basename | sed -e 's/^.*\.//' \
		-e y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/`

	# Select output file.
	case $testrun in
	1) script="/dev/tty" ;;
	*) script="$basename.IX" ;;
	esac

	# Select category for classification entries.
	case $suffix in
	1C)	sect="networking and communication commands" ;;
	1V)	sect="System V-specific commands" ;;
	1*) sect=commands ;;
	2*) sect="system calls" ;;
	3C)	sect="compatibility library" ;;
	3K)	sect="kernel virtual-memory library" ;;
	3L)	sect="lightweight processes library" ;;
	3N)	sect="networking functions" ;;
	3M)	sect="mathematics library" ;;
	3R)	sect="RPC library" ;;
	3S)	sect="standard I/O functions" ;;
	3V)	sect="System V-specific libraries and functions" ;;
	3X)	sect="misc. libraries" ;;
	3*) sect="library functions" ;;
	4F)	sect="protocol families" ;;
	4N)	sect="network interfaces" ;;
	4M)	sect="\s-1STREAMS\s0 modules" ;;
	4P)	sect="protocols" ;;
	4*) sect="devices" ;;
	5V)	sect="System V-specific versions" ;;
	5*) sect="file formats" ;;
	6*) sect="games and demos" ;;
	7*) sect="miscellaneous tables and macros" ;;
	8V)	sect="System V-specific commands" ;;
	8C)	sect="networking and communication commands" ;;
	8S)	sect="standalone \s-1PROM\s0 utilities" ;;
	8*) sect="commands" ;;
	*)  sect="misc" ;;
	esac

	# Build head of shell script with here document.
	case $testrun in
	1) echo "" > $base.IX ;;
	*) : ;;
	esac
	echo "ed - $basename <<'END.$basename'" >> $script
	echo 'g/^\.IX/d' >> $script
	echo "/^.SH DESCRIPTION/a" >> $script
	echo '.\" pix' >> $script


	# If trailer contains "(...online...)", it's a see also entry,
	# so edit trailer to reflect this in the alternate-name entries
	# below.
	trailer="`getNAME -r $1 | sed -e 's/^.*\\\- //'`"
	if test -z "`echo $trailer | grep '(.*online.*)'`"
	then
		see=0
		# Add basic index entries from name lines.
		# yank out parenthetical remarks
		trailer="`echo $trailer | sed 's/(.*) //g'`" 
		echo \
".IX $base \"\" \"\\&\\fL$base\\fR($suffix) \\(em $trailer\"" >> $script
		echo \
".IX \"${sect}\" $base \"\" \"\\&\\fL$base\\fR($suffix) \\(em $trailer\"" >> $script
		echo "$base	$1" >> $database
	else
		see=1
trailer="PRINT \"see \\&\\fL`echo $base | sed 's/_.*$//'`\\fR($suffix)\""
	fi

	# Add index entries for alternate names from getNAME -a.
	for j in `getNAME -a $1 | awk '{ print $1 }'`
	do
		basename=`basename $j`
		base=`echo $basename | sed 's/\..*$//'`
		case $see in
		0)	# A regular entry with alternate names.
			echo \
".IX $base \"\" \"\\&\\fL$base\\fR \\(em $trailer, in \\&\\fL$file\\fR($suffix)\"" >> $script
			echo \
".IX \"${sect}\" $base \"\" \"\\&\\fL$base\\fR \\(em $trailer, in \\&\\fL$file\\fR($suffix)\"" >> $script
		echo "$base	$1" >> $database
		;;
		1)	# A see also entry.
			echo \
".IX $base \"\" \"\\&\\fL$base\\fR\"  \"\" $trailer" >> $script
			echo \
".IX \"${sect}\" $base \"\" \"\\&\\fL$base\\fR\" $trailer" >> $script
		echo "$base	$1" >> $database
		;;
		esac
	done

	# Reset original basename.
	basename=`basename $1`
	base=`echo $basename | sed -e 's/\..*$//' -e 's/Intro/intro/'`

	# Display rotated regular entries.
	case $see in
	0)
		for j in $trailer
		do
			front=`echo $trailer | sed "s@.*$j @@"`
			first=`echo $front | awk '{ print $1 }'`
			# Weed out commas.
			case $first in
			,)	front=`echo $trailer | sed "s@.*, @@"`
				first=`echo $front | awk '{ print $1 }'`
			;;
			esac
			back=`echo $trailer | sed "s@ *$front *@@"`
			# Add comma to end if rotated.
			case "$back" in
			'')	
			;;
			*)	front="${front},"
				back="$back "
			;;
			esac
			# Weed out stupid cases.
				case $first in
				-*|a|about|an|as|at|be|by|do|for|from|go|if|in|is|it|my|of|on|or|so|to|and|any|are|but|can|may|non|nor|not|our|the|too|two|via|was|who|you|also|been|both|come|done|each|from|have|keep|kept|many|more|most|much|near|need|none|only|onto|over|seem|some|soon|stay|such|than|that|the|them|they|this|thus|upon|very|well|went|were|what|when|whom|will|with|your)
				;;
				*)	echo \
".IX \"${front} ${back}\\(em \\&\\fL$base\\fR($suffix)\"" >> $script
				;;
				esac
		done
	;;
	esac
	echo '.\"' >> $script
	echo "." >> $script

	# Produce FILES entries.
	trap '/bin/rm Pix.tmp; exit' 1 2 5 9 15
	getNAME -S FILES $1 | grep "." > Pix.tmp
	count=`cat Pix.tmp | wc -l`
	if test $count -gt 0
	then 
		echo "/^.SH FILES/a" >> $script
		echo '.\" pix' >> $script
		number=1
		while test $number -le $count
		do
			line=`sed -n -e ${number}p -e "s/([\*][\*]/*/" Pix.tmp`
			item=`echo $line | awk '{ print $1 }'` 
			clean=`echo "$item" \
				| sed -e 's@[.\~/*()$]@@g' -e "s@^\.*@@"`
			info=`echo "$line" \
				| sed -e "s@$item *@@" -e 's/[\.!$]$//'`
			if test ! -z "$info" ; \
			then info="${info} \\(em" ; \
			else info="in" ; \
			fi
			echo \
".IX $clean \"${info} \\&\\fL${base}\\fR(${suffix})\" \"\\&\\fL$item\\fR\"" >> $script
#			echo \
#".IX files ${clean}$base \"\" \"\\&\\fL$item\fR \\(em ${info} \\&\\fL${base}\\fR(${suffix})\"" >> $script
			echo "$item	$1" >> $database
			number=`expr $number + 1`
		done
		echo '.\"' >> $script
		echo "." >> $script
	fi
	/bin/rm Pix.tmp

	trap '/bin/rm Pix.tmp; exit' 1 2 5 9 15
	getNAME -rS DIAGN $1 | sed -n -e '/^\.TP/N' -e '/^\.TP.*/p' \
		| sed "/^\.TP/d" | deroff | sed 's/^ *//' > Pix.tmp
	count=`cat Pix.tmp | wc -l`
	if test $count -gt 0
	then 
		echo "/^.SH.*DIAGN/a" >> $script
		echo '.\" pix' >> $script
		number=1
		while test $number -le $count
		do
			line=`sed -n -e ${number}p Pix.tmp`
			item=`echo $line | awk '{ print $1 }'` 
			clean=`echo "$item" \
				| sed -e 's@[.\~/*()$]@@g' -e "s@^\.*@@"`
			info=`echo "$line" \
				| sed -e "s@$item *@@" -e 's/[\.!$]$//'`
			if test ! -z "$info" ; then info=" ${info}," ; fi
#			echo \
#".IX \"$line \\(em diagnostic from \\&\\fL${base}\\fR(${suffix})\"" >> $script
			echo \
".IX \"diagnostic messages\" \"\\&\\fL$line\\fR \\(em \\&\\fL${base}\\fR(${suffix})\"" >> $script
			echo "$item	$1" >> $database
			number=`expr $number + 1`
		done
		echo '.\"' >> $script
		echo "." >> $script
	fi
	/bin/rm Pix.tmp

	# Index subcommands, if any
	if test ! -z "`grep '^\.SS.*Commands' $1`"
	then
		echo "/^.SS.*Commands/a" >> $script
		echo '.\" pix' >> $script
		echo \
".IX subcommands $base \"\" \"for \\&\\fL$base\\fR($suffix)\"" >> $script
		getNAME -rS USAGE $1 | sed -n "/.SS.*Commands/,/^\.S[SH]/p" \
			| sed -e "/^\.S/d" \
			-e "/^\.RS/,/^\.RE/d" \
			| sed -n -e /.[TH]P/N \
			-e "/\.[TH]P.*/p" \
			-e "/.IP/p" \
			| ( echo .nh ; echo .TH ; cat ) \
			| troff -man -a | awk '{ print $1 }' \
			| sed -e "s/\[.*//" -e "/^[+-.(%A-Z]/d" \
			| sort -u > Pix.tmp
		for i in `cat Pix.tmp`
		do 	if test ! -z "`grep '^\.R*B.*'$i $1`"
			then
				echo \
".IX ${i}$base \"\" \"\\&\\fL$i\\fR, in \\&\\fL${base}\\fR($suffix)\"" >> $script
			fi
		done
		echo '.\"' >> $script
		echo "." >> $script
	fi

	# Index input grammer, if any
	if test ! -z "`grep '^\.SS.*Input Grammer' $1`"
	then
		echo "/^.SS.*Input Grammer/a" >> $script
		echo '.\" pix' >> $script
		echo \
".IX \"grammatical constructs\" $base \"\" \"for \\&\\fL$base\\fR($suffix)\"" >> $script
#		echo \
#".IX \"input grammmer\" $base \"\" \"for \\&\\fL$base\\fR($suffix)\"" >> $script
		echo \
".IX syntax $base \"\" \"of \\&\\fL$base\\fR($suffix)\"" >> $script

		getNAME -rS USAGE $1 | sed -n "/.SS.*Input Grammer/,/^\.S[SH]/p" \
			| sed -e "/^\.S/d" \
			-e "/^\.RS/,/^\.RE/d" \
			| sed -n -e /.[TH]P/N \
			-e "/\.[TH]P.*/p" \
			-e "/.IP/p" \
			| ( echo .nh ; echo .TH ; cat ) \
			| troff -man -a | awk '{ print $1 }' \
			| sed -e "s/\[.*//" -e "/^[+-.(%A-Z]/d" \
			| sort -u > Pix.tmp
		for i in `cat Pix.tmp`
		do 	if test ! -z "`grep '^\.R*B.*'$i $1`"
			then
				echo \
".IX ${i}$base \"\" \"\\&\\fL$i\\fR construct, see \\fICommands\\fR in \\&\\fL${base}\\fR($suffix)\"" >> $script
			fi
		done
		echo '.\"' >> $script
		echo "." >> $script
	fi

	# Index variables, if any
	if test ! -z "`grep -i '^\.SS.*Variables' $1`"
	then
		echo "/^.SS.*Variables/a" >> $script
		echo '.\" pix' >> $script
		echo \
".IX variables $base \"\" \"for \\&\\fL$base\\fR($suffix)\"" >> $script
		getNAME -rS USAGE $1 | sed -n "/.SS.*Variables/,/^\.S[SH]/p" \
			| sed -e "/^\.S/d" \
			-e "/^\.RS/,/^\.RE/d" \
			| sed -n -e /.[TH]P/N \
			-e "/\.[TH]P.*/p" \
			| ( echo .nh ; echo .TH ; cat ) \
			| troff -man -a | awk '{ print $1 }' \
			| sed -e "s/\[.*//" -e "/^[+-.(%A-Z]/d" \
			| sort -u > Pix.tmp
		for i in `cat Pix.tmp`
		do 	if test ! -z "`grep '^\.R*B.*'$i $1`"
			then
				echo \
".IX ${i}$base \"\" \"\\&\\fL$i\\fR variable, see \\fIVariables\\fR in \\&\\fL${base}\\fR($suffix)\"" >> $script
			fi
		done
		echo '.\"' >> $script
		echo "." >> $script
	fi

	# Index envars, if any
	if test ! -z "`grep -i '^\.SH.*ENVIRON' $1`"
	then
		echo "/^.SH.*ENVIRON/a" >> $script
		echo '.\" pix' >> $script
		echo \
".IX \"environment variables for\" $base \"\" \"\\&\\fL$base\\fR($suffix)\"" >> $script
		getNAME -rS USAGE $1 | sed -n "/.SH.*ENVIRON/,/^\.S[SH]/p" \
			| sed -e "/^\.S/d" \
			-e "/^\.RS/,/^\.RE/d" \
			| sed -n -e /.[TH]P/N \
			-e "/\.[TH]P.*/p" \
			| ( echo .nh ; echo .TH ; cat ) \
			| troff -man -a | awk '{ print $1 }' \
			| sed -e "s/\[.*//" -e "/^[+-.(%A-Z]/d" \
			| sort -u > Pix.tmp
		for i in `cat Pix.tmp`
		do 	if test ! -z "`grep '^\.R*B.*'$i $1`"
			then
				echo \
".IX ${i}$base \"\" \"\\&\\fL$i\\fR environment variable, see \\s-1IENVIRONMENT\\s0 in \\&\\fL${base}\\fR($suffix)\"" >> $script
			fi
		done
		echo '.\"' >> $script
		echo "." >> $script
	fi

	# Produce entries for nonstandard subsection headers.
	grep "^\.SS" $1 | sed \
		-e "/Commands/d" \
		-e "/Variables/d" \
		-e "/Grammar/d" \
		-e "s/^\.SS *//" \
		-e "s/\"//g" \
		> Pix.tmp
	if test -s Pix.tmp
	then
		count=`cat Pix.tmp | wc -l`
		number=1
		while test $number -le $count
		do
			line=`sed -n -e ${number}p Pix.tmp`
			loc=`echo $line | sed -e 's@ @.*@g' -e 's@/@.@g' \
				-e 's@"@@g'`
			clean=`echo $line | deroff | sed -e "s/ //g"`
			topic=`echo $line | deroff | sed \
			y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/`
			echo "/.SS.*$loc/a" >> $script
			echo '.\" pix' >> $script
			echo \
".IX ${clean}${base} \"\" \"$topic, in \\&\\fL${base}\\fR($suffix)\"" >> $script
#			echo \
#".IX topics \"$clean\" \"\" \"$topic, in \\&\\fL${base}\\fR($suffix)\"" >> $script
			echo '.\"' >> $script
			echo "." >> $script
			number=`expr $number + 1`
		done
	fi

	# Produce tail end of script.
	echo "w" >> $script
	echo "q" >> $script
	echo "END.$basename" >> $script
	shift
done
