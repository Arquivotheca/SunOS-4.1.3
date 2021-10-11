#! /bin/sh
#
#%Z%%M% %I% %E% SMI
#
######################################################################
# This utility facilitates the migration of build fixes from an
# integration machine to the various build machines.  When a build 
# problem gets fixed, the lead release engineer should:
# 
# 1)	login to an INTEGRATION machine
# 
# 2) 	Create the input file '$top/$sid_dir/sidfix', which will be the
#	basis for editing SIDlist on the integration machine, and 
#	extracting/deleting files from the source tree on the build 
#	machines.  This file should list the sources relative to $top 
#	(usually /usr/src), followed by white space and SID number.
# 	eg:
#		subdir/SCCS/s.sourcefile 1.7
#		subdir/subdir2/SCCS/s.sourcefile 1.2
#
#	To earmark a file for deletion, the filename should be preceded by 
#	*D* <<no white space!, SID# optional >> as in:
#
#		*D*subdir/subdir3/SCCS/s.sourcefile
# 
# 3)	Execute 'patch_sid [-n] [-l]'. 
#	-n turns off sending mail to other RE's such as when 
#		lang/boot and libs are first built by one RE.
#	-l creates a list of sidfix entries that show no change
#		from the current SIDlist.  This option is most
#		useful for PSR releases where sidfix entries are
#		generated based on a snapshot of entire directories
#		that have been smerged.  If the -l option is not
#		used, the script asks for confirmation to include
#		sidfix entries that haven't changed.
# 
# After 'patch_sid' successfully executes on the integration machine,
# it sends mail to the release engineers, who should then 
# follow these steps:
# 
# 1)	login to their BUILD machines
# 
# 2)	Create the input file '$top/$sid_dir/sidfix', by
# 	mousing up the contents of the received e-mail.
# 
# 3)	Execute 'patch_sid' to populate their source trees
# 	with the build fixes, or remove files deleted from SIDlist.
# 
# 4)	Rebuild affected directories.
#
# Since the SIDlist build philosophy requires that the SCCS_DIRECTORIES
# not be mounted when a 'make' may be running, a separate entry similar 
# to this is required in the BUILD machine's fstab:
#
# For the ***4.1 BUILD MACHINES ONLY*** the entry is:
# aprime:/usr/bigsrc.41/SCCS_DIRECTORIES /mnt/41src nfs ro,bg,hard,intr  0 0
#
# In general, the script mounts/unmounts the SCCS sources as needed
# and verifies the input file data.  After execution, it renames the
# input file by suffixing it with the new SID of SIDlist, thus 
# providing some kind of audit trail.  Each release engineer can
# check that her/his build fixes are up-to-date by looking for a complete
# set of suffixed files.
# 
# The script also performs these functions, depending on where it 
# is invoked:
# 
#	Integration Machine:  edits the SIDlist using the input
#	file, and checks it back into SCCS with comments entered
#	on the keyboard, followed by a Control-D.  To sidestep
#	various internal size limits in the use of 'sed', sidfix
#	files containing more than 80 lines are split up.
#
#	Build Machines:  extracts/removes the sources listed in
#	the input file.  [Options -n -l are ignored].
#
# NOTE: This script can create and delete entries to SIDlist, as well as
#	change existing ones.  Where a path or filename change is desired,
#	create a 'delete' entry for the old and another one for the new.
#	This script may have to be modified for concurrent use of one set
#	of integration sources by multiple releases, as may be the
#	case for 4.1 and 4.1.1.
#
#
######################################################################

PWD=`pwd`
fix=sidfix			#list of modified files and deltas
bindir=/usr/release/bin
top=/usr/src
reng="austin@nova alison@gaia stub@mousetrap"
bugid='patch_sid'

usage="USAGE:  $0 [-n] [-l]"
if [ $# -gt 1 ] ; then 
	echo "$usage" && exit 1
else
	case $1 in
		-n )	reng=`whoami`@`hostname`;; 
		-l )	list_chg=1;;
		'' ) 	;;			
		*  )	echo $usage && exit 1;;
	esac
fi

tmp=/tmp; sid_dir=$top/SIDlist

if [ ! -f $sid_dir/$fix ] ; then
	echo "You must create '$sid_dir/$fix' by listing the modified"
	echo "source files and deltas required to complete the build."
	exit 1
fi

#Determine which function will be invoked

case `hostname` in

	solaris)
		edit_sid=1    #Integration machines
		mt_dir=/usr/src/SCCS_DIRECTORIES;;
	krypton|colander|godzilla)
		get_files=1   #Build machines
		mt_dir=/mnt/413src;;
	* )
		echo "`hostname` is not a known release machine!" && exit 1	
esac

#Get mount state

/etc/mount | grep -s " $mt_dir " || unmounted=1 

#Screen yes/no reponses

get_yn='(while read yn; do
	case "$yn" in
		[Yy]* ) exit 0;;
		[Nn]* ) exit 1;;
		    * ) echo -n "Please answer y or n:  ";;
	esac
done)'

#Mount and unmount partitions

get_mtstat='( [ "$do_mt" = "0" ] && cmd=".mountit" || cmd="unmount"
done=""
while :
do
	/etc/mount|grep -s " $mt_dir "
	[ "$?" = "$do_mt" ] && exit 0
	if [ "$done" ]; then
		echo -n "Unable to $cmd $mt_dir!  Try again? (y/n)  "
		eval "$get_yn" && continue || exit 1
	fi
	${bindir}/${cmd} $mt_dir
	done=1
done )'

#Clean up before exiting

sv_errs=$tmp/patch_sid.errs; rm -f $sv_errs
nochg=$tmp/no_change; rm -f $nochg
errs=$tmp/err_$$

cleanup='( [ "$edit_sid" ] && [ -f $sid_dir/SCCS/p.${sid} ] && \
	sccs unedit $sid;
	if [ -s $errs ]; then
		cat $errs >  $sv_errs; echo "ERROR!"
		echo "Errors are in $sv_errs."
	fi
	if [ -s $nochg ]; then 
		echo "Any files listed in $nochg should be investigated."
		echo "They match entries already in $sid."
	fi
rm -f $tmp/*$$*; cd $PWD
if [ "$unmounted" ]; then
	do_mt=1
	eval "$get_mtstat" && exit 0 || exit 1
fi )'

do_mt=0
eval "$get_mtstat" || exit 1

trap 'echo ABORTED!; eval "$cleanup"; exit 1' 1 2 3 15

#NOTE:  Need to change this for branched releases!!      <<<<<<<<<<<<<

if [ "$edit_sid" ]; then
	(cd $top/sys/conf.common; sccs get -s RELEASE)   
else
	(cd $top/sys/conf.common 
	sccs get -s ${mt_dir}/sys/conf.common/RELEASE)
fi
release=`cat $top/sys/conf.common/RELEASE`
sid=${release}.SID

#Check if $sid is editable; also clean up $fix

cd $sid_dir
if [ "$edit_sid" ]; then
	if [ -f SCCS/p.${sid} ]; then
		echo "Writable $sid exists!"
		edit_sid=""
		eval "$cleanup"; exit 1
	fi
	sed -e '/^[ 	]*$/d' \
	-e "/^SIDlist\/SCCS\/s.$sid /d" $fix | sort |uniq > \
	$tmp/${fix}_$$
else
	sed -e '/^[ 	]*$/d' $fix | sort |uniq > $tmp/${fix}_$$
fi

#Validate $fix; separately process delete/change/add

echo "Verifying '$fix' entries..."
cat $tmp/${fix}_$$ | \
awk '{ if (NF > 2 ) printf ("Too many fields: %s\n", $0) }' > $errs

egrep -v 'SCCS/s.' $tmp/${fix}_$$ > $tmp/$$
if [ -s $tmp/$$ ]; then
	echo "These entries should be in 'SCCS/s.' format:" >> $errs
	cat $tmp/$$ >> $errs
fi

if [ -s $errs ] ; then
	eval "$cleanup"; exit 1
fi

[ "$edit_sid" ] && sccs get $sid 
del=$tmp/del_$$; rest=$tmp/rest_$$; add=$tmp/add_$$; chg=$tmp/chg_$$
current=$tmp/current_$$ todo=$tmp/todo_$$
grep '^*D*' $tmp/${fix}_$$ | sed 's/*D*//g' | awk '{print $1}'  > $del
grep -v '^*D*' $tmp/${fix}_$$ | awk '{print $1, $2}' > $rest

notfound='No such file: '; rm -f $errs; ask=$tmp/ask_$$

#Get entries that are the same in SIDlist-- build a list
#if '-l' option used; also verify that SID really exists.

if [ "$edit_sid" ]; then
	cat $rest | while read line; do
		if [ "$list_chg" ]; then
			egrep "^${line}$" $sid >> $nochg && continue
		else
			egrep "^${line}$" $sid >> $ask && continue
		fi
	set $line
	sccs get -g -r$2 ${mt_dir}/$1  1>> /dev/null 2>> $errs && \
	echo "${line}" >> $todo && continue
	done	
else
	cp -p $rest $todo
fi

#If '-l' not used, ask if ok to process entries that haven't changed.

if [ -s $ask ] ; then
	echo "Line(s) below reflect no change from current SIDlist."
	cat $ask |more
	echo -n "Do you wish to process them anyway? (y/n)  "
	if eval "$get_yn"; then
		cat $ask > $chg 
	else
		echo "" >> $errs
		echo "Remove line(s) that show no change from $sid" >> $errs
		cat $ask >> $errs
		echo "" >> $errs
		echo "Verifying remaining entries..."
	fi
fi

cat $rest| awk '{ if ($2 !~ /^[0-9]+\.[0-9]+/)
	printf("Bad format: %s %s\n", $1, $2) }' >> $errs

#Get confirmation about new files to be added

if [ -s $todo ] ; then
   for i in `cat $todo | awk '{print $1}'`; do
	if [ "$edit_sid" ]; then 	#check vs. current $sid (patience!)
		if egrep -s "^$i[ 	]" $sid; then
			egrep "^$i[ 	]" $todo >> $chg;  continue
		fi
		echo "This file is not in $sid:" 
		echo "  $i"
		echo -n "Add new entry?  (y/n)  "
		eval "$get_yn" && egrep "^$i[ 	]" $todo >> $add && continue
		echo "Remove entry:  $i " >> $errs
	else				#OK to check vs SCCS tree (faster)
		[ -f ${mt_dir}/$i ] && continue
		echo "$notfound $i" >> $errs
	fi
   done
fi

#Check deletes

if [ -s $del ] && [ "$edit_sid" ]; then
	for i in `cat $del`; do
		[ -f $top/$i ] && continue
		egrep -s "^$i[ 	]" $sid && continue
		echo "$notfound $i" >> $errs
	done
fi

if [ -s $errs ] || [ -s $nochg ] ; then 
   if [ "$edit_sid" ] ; then
	ok=/tmp/patch_sid.pass; rm -f $ok
	for i in $chg $del $add; do
		if [ "$i" = $del ] && [ -s $del ] ; then
			for j in `cat $del`; do
				egrep -s "[ 	]$j$" $errs && continue
				echo "*D*$j" >> $ok && continue
			done
		else
			[ -s $i ] && cat $i >> $ok
		fi
	done
	echo ""
	eval "$cleanup"
	[ -s $ok ] && echo "Entries that have passed verification are in '$ok'."
	exit 1
   else
	eval "$cleanup"; exit 1
   fi
fi

#Get next SID for $sid; use it to rename $fix 

if [ "$edit_sid" ]; then
	comfile=$tmp/comment_$$
	echo \
	"Enter delta comments below; type Control-D when done:"
	while read comment; do
		echo "$comment">> $comfile
	done
	[ "$bugid" ] && echo "$bugid" >> $comfile
	echo "Editing large file '$sid'... "
	trap 'echo ABORTED!; eval "$cleanup"; exit 1' 1 2 3 15
	if sccs edit $sid > $tmp/ed_$$ ; then
		:
	else
		eval "$cleanup"; exit 1
	fi
	new_sid=`fgrep 'new delta' $tmp/ed_$$ | awk '{print $3}'`
	echo "New version is $new_sid..."
else
	new_sid=`fgrep "SIDlist/SCCS/s.$release.SID" $todo |\
	awk '{print $2}'`
	if [ ! "$new_sid" ] ; then 
		echo "ERROR: Input file should include $sid file."
		eval "$cleanup"; exit 1
	fi
fi

#$newfix shouldn't already exist; check it just the same 

newfix=${sid_dir}/${fix}_${new_sid}
if [ -s $newfix ]; then 
	echo -n "$newfix exists; Clobber? (y/n)  "
	if eval "$get_yn"; then
		rm -f $newfix
	else
		[ "$edit_sid" ] && eval "$cleanup"; exit 1
	fi
fi

#If invoked on integration system, edit $sid; fix format of new files;
#split huge sedfiles to avoid overflow;
#append $bugid to sccs delta comments; Send mail

if [ "$edit_sid" ]; then
	stars='********************'; sedit=$tmp/sedit_$$; sidtmp=$tmp/sid_$$
	if [ -s $chg ]; then
		cat $chg | \
		awk '{printf ("s@^\(%s \)\(.*\)@\1%s@\n", $1, $2)}' > $sedit
	fi
	if [ -s $del ]; then
		cat $del | \
		awk '{printf ("s@^\(%s \)\(.*\)@@g\n", $1 )}' >> $sedit
		echo '/^[ \t]*$/d' >> $sedit
	fi
	if [ -s $sedit ]; then
		maxsz=80
		sz=`wc -l $sedit | awk '{print $1}'`
		if [ $sz -gt $maxsz ] ; then
		   subset=$tmp/split$$_; nu=$tmp/newcmd$$
		   split -$maxsz $sedit $subset
		   sets=`ls $subset*`; first=1
		   for i in $sets; do
			sfx=`echo $i | sed 's@^.*_@@g'`
			interm=$tmp/${sfx}_$$
			if [ "$first" ]; then
			   echo "sed -f $i $sid 2>> $errs >> $interm" >> $nu
			   echo 'sync; sync' >> $nu
			   echo "rm -f $sid" >> $nu
			   first=
			else	
			   echo "sed -f $i $nextsid 2>> $errs >> $interm" \
			   >> $nu
			   echo 'sync; sync' >> $nu
			   echo "rm -f $nextsid" >> $nu
			fi
			nextsid=$interm
		   done
		   chmod +x $nu; . $nu 2>> $errs ; sync; sync
		   [ ! -s $errs ] && mv $interm $sidtmp 2>> $errs 
		else
		   sed -f $sedit $sid > $sidtmp 2>> $errs
		fi
	else
		cp -p $sid $sidtmp 2>> $errs
	fi
	if [ -s $add ]; then
		cat $add | awk '{print $1, $2}' >> $sidtmp 2>> $errs
	fi
	sync; sync
	[ -s "$errs" ] && echo "EDIT failed!" && eval "$cleanup" && exit 1
	mv $sidtmp $sid; sync; sync
	trap '' 1 2 3 15
	sed -e 's/$/ \\/g' $comfile | sccs delget $sid
	if [ ! "$?" = 0 ] ; then
		echo "^GUnable to delta $sid!" 
		eval "$cleanup"; exit 1
	fi
	rm -f ${fix}_save; mv $fix ${fix}_save
	cp -p $tmp/${fix}_$$ $fix
	trap 'echo ABORTED!; eval "$cleanup"; exit 1' 1 2 3 15
	echo -n "Include special instructions in e-mail? (y/n)  "
	eval "$get_yn" && do_mesg=1
	ed $tmp/${fix}_$$ > /dev/null 2>&1 << jlt
1i
Please follow these steps to populate your source tree
with the latest build fixes for $release.

1.  create '$sid_dir/$fix' using the entries 
    below the dotted line
2.  patch_sid
3.  rebuild affected directories.

After extracting the SCCS files using 'patch_sid', '~/$fix' 
on your build machine will be moved to '~/${fix}_$new_sid'.  Please 
check that we are in sync by verifying that you have the 
complete set of suffixed files in $sid_dir.

`whoami`@`hostname`
---------------------cut here-------------------------
SIDlist/SCCS/s.${sid} ${new_sid}
.
w
q
jlt
	mesg=$tmp/mesg_$$
	if [ "$do_mesg" ] ; then
		echo "${stars}SPECIAL INSTRUCTIONS${stars}
" 		> $mesg
		echo "${stars}${stars}${stars}
		">> $mesg
		echo "SIDlist delta comments are:" >> $mesg
		cat $comfile >> $mesg
		echo "" >> $mesg
		cat $tmp/${fix}_$$ >> $mesg
		echo -n \
"Press <RETURN> to invoke 'vi'; your text will be prepended to usual mail: "
		read return
		vi +2 $mesg
	else
		echo "SIDlist delta comments are:" >> $mesg
		cat $comfile >> $mesg
		echo "" >> $mesg
		cat $tmp/${fix}_$$ >> $mesg
	fi
	echo "Sending mail to:"
	echo "	$reng"
	/usr/ucb/mail -s "NEW sidfix $new_sid" $reng < $mesg
	mv $fix $newfix; chmod +w $newfix
fi

#If invoked from a build machine, extract/delete the sources in $fix

if [ "$get_files" ]; then
	cd $top
	getit=$tmp/getit_$$; bad=$tmp/bad_$$;
	echo "cd $top" > $getit
	cat $todo | \
	sed -e 's/\/SCCS\/s\./  /g' -e 's/^SCCS\/s\./\. /g' |\
        awk '{printf ("(cd %s; sccs get -r%s ${mt_dir}/%s/%s) 2>>$errs \n",\
	$1, $3, $1, $2 )}' >> $getit
	if [ -s $del ]; then
		cat $del | \
		sed -e 's/\/SCCS\/s\./  /g' -e 's/^SCCS\/s\./\. /g' |\
		awk '{printf ("( [ -d %s ] && cd %s && rm -f %s ) 2>> $errs\n",\
		$1, $1, $2 )}' >> $getit
	fi
	chmod +x $getit
	trap '' 1 2 3 15
	. $getit
	[ -s $errs ] && grep -v 'No id keywords' $errs > $bad
	if [ -s $bad ]; then
		mv $bad $errs
	else
		rm -f $errs
		mv ${sid_dir}/${fix} $newfix; chmod +w $newfix
	fi
fi

[ -s $newfix ] && touch $newfix && chmod 444 $newfix
eval "$cleanup" && exit 0 || exit 1

