:
# @(#)metaclean.sh	1.1
  ##  purpose:	Clean SCCS scratch area, with the option of
  ##		running 'make clean', 'sclean' & 'editfiles'.
  ##		If this utility is to be used to remove ONLY 'garbage'
  ##		or scratch files (eg. make.out .makestate .errs foo),
  ##		answer "no" to 'make clean' and 'sclean' questions,
  ##		then edit out the list containing object files.
  ##		
  ##  remarks:	o Links to SCCS_DIRECTORIES are NOT removed by this
  ##		  utility.
  ##		o This utility can be used by itself, although it is 
  ##		  also invoked from 'do_upgrade' and  'do_crankturn' scripts

PWD=`pwd`
#Restore original mount state when exiting
trap 'echo "metaclean ABORTED!"
[ "$UNMOUNTED" ] && DO_MT=1 || DO_MT=0
eval "$GET_MTSTAT"
cd $PWD
exit 1' 1 2 3 15

[ ! `whoami` = "root" ] && echo "Superuser required" && exit 1
USAGE="USAGE: $0"
[ ! "$#" = 0 ] && echo $USAGE && exit 1
GET_YN='(while read yn
do
    case "$yn" in
    [Yy]* ) exit 0;;
    [Nn]* ) exit 1;;
        * ) echo -n "Please answer y or n:  ";;
esac
done)'

GET_MTSTAT='( [ "$DO_MT" = "0" ] && CMD="mount" || CMD="umount"
DONE=""
while :
do
	/etc/mount|fgrep -s $MT_DIR
	[ "$?" = "$DO_MT" ] && exit 0
	if [ "$DONE" ]
	then
		echo -n "Unable to $CMD $MT_DIR!  Try again? (y/n)  " 
		while read yn
		do
		case $yn in
			[Yy]* ) break;;
			[Nn]* ) exit 1;;
			    * ) echo -n "Please answer y or n:  ";;
		esac
		done
	fi
	echo "/etc/${CMD} $MT_DIR..."
	/etc/${CMD} $MT_DIR  
	DONE=1
done )'

HOST=`hostname`
if [ ! $PWD = "/usr/src" ] 	#can be used on any directory
then
	echo -n "Directory to clean is $PWD!  Continue? (y/n)  "
	eval "$GET_YN" || exit 1
fi
/etc/mount|grep -s SCCS_DIRECTORIES || UNMOUNTED=1  #Flag to restore mount state
MT_DIR='/usr/src/SCCS_DIRECTORIES'
DO_MT=0

while echo -n "Search for SCCS files being edited? (y/n)  " #optional
do
	eval "$GET_YN" || break
	eval "$GET_MTSTAT" || break
	echo "Executing 'editfiles|tee -i /tmp/editfiles.out'..."
	/usr/release/bin/editfiles | tee -i /tmp/editfiles.out 2>&1
        echo -n "Continue?  (y/n)  "
        eval "$GET_YN" || exit 1
        break
done

while echo -n "Execute 'make -k clean'? (y/n)  " 	#optional
do
	eval "$GET_YN" || break
	eval "$GET_MTSTAT" || break
	echo "Executing 'make -k clean'..." && sleep 1
	make -k clean
	break
done

while echo -n "Execute 'sclean'? (y/n)  "     #optional	
do 
        eval "$GET_YN" || break 
	eval "$GET_MTSTAT" || break
        echo "Executing 'sclean|tee -i sclean.out'..." && sleep 1
       	/usr/release/bin/sclean| tee -i sclean.out 2>&1 
	break
done 
DO_MT=1
eval "$GET_MTSTAT"

echo "You have the option of naming the release engineer for
the previous build, assuming that ALL files owned by that user
are intermediate files that can be removed."
while echo -n "Enter user name for previous build, or <CR> to ignore:  "
do
	read reng
	case x$reng in
	    x) ADD_SEARCH="-size 0"; break;;
	    *) grep -s "^$reng:" /etc/passwd && \
	       ADD_SEARCH="-user $reng -o -size 0"&& break
	       echo "User '$reng' not found";;
	esac
done	

TEMP=/tmp/$$
for action in list remove
do
	echo -n "Proceed to $action $PWD files on '$HOST'?  (y/n) "
	eval "$GET_YN" 		
	if [ "$?" = "1" ] 
	then 
		[ ! "$UNMOUNTED" ] && DO_MT=0 && eval "$GET_MTSTAT"
		exit 1
	fi
	echo "Please wait..."

#PARAMETERS for list building are:
#don't descend into "*SCCS*"
#remove all files owned by $reng (if supplied), and all zero sized files
#remove all files NOT named Makefile or *.{c,s,sh,h,y,r,f,p,l}
	if [ "$action" = "list" ]
	then
	   find `ls -a | sed -e /SCCS/d  -e '/^\.$/d' -e '/^\.\.$/d'` \
	   ! -name SCCS  \( $ADD_SEARCH -o ! \( -name "*.c" -o -name "*.s" \
	   -o -name "*.sh" -o -name "*.h" -o -name "Makefile" -o -name "*.y" \
	   -o -name "*.r" -o -name "*.f" -o -name "*.p" -o -name "*.l" \
	   \) \) \
	   -type f -exec ls -l >> $TEMP {} \; 

	   [ ! -s "$TEMP" ] && \
	   echo "No extraneous files found for removal." && break	
	   RM_LIST=""
	   grep '\.o$' $TEMP > $TEMP.OBJECTS	#isolate object files	
	   grep -v '\.o$' $TEMP > $TEMP.REST	#from the REST
	   rm -f $TEMP		#discard intermediate files
	   for eachlist in $TEMP.OBJECTS $TEMP.REST  # and empty lists
	   do
		   [ ! -s $eachlist ] && rm -f $eachlist && continue
	           RM_LIST="$RM_LIST $eachlist"
	   done
	   echo \
"	The files that will be removed by this utility are
	listed in: $RM_LIST.  
	The list(s) will be used as input to 'rm -f'.  
	If you wish to SAVE some listed files, 
	please DELETE the appropriate lines NOW!"
	   for eachlist in $RM_LIST	#edit remaining list/s
	   do
		echo -n "Review/EDIT $eachlist? (y/n)  "
		eval "$GET_YN" || continue
		vi $eachlist
		set `wc -c $eachlist` #all lines deleted=1byte left
		[ "$1" -gt 1 ] || rm -f $eachlist  #remove it
	   done		       
       	   [ ! -f $TEMP.OBJECTS ] && [ ! -f $TEMP.REST ] && break 
	fi			#If any lists remain, proceed to 'remove'

	if [ "$action" = "remove" ]
	then
		for eachlist in `ls $TEMP.*`
		do
			cat $eachlist | awk '{print $8 > "/tmp/awk.out"}'
			cat /tmp/awk.out | while read line
			do
				rm -f $line
			done
		done
		rm -f /tmp/awk.out
	fi
done

echo -n "List remaining source/saved files? (y/n)  "
eval "$GET_YN"
if [ "$?" = "0" ]
then
#Now get a list of sources and and saved files
	echo "Looking for remaining source/saved files..."
	find `ls -a | sed -e /SCCS/d  -e '/^\.$/d' -e '/^\.\.$/d'` \
	! -name SCCS -type f  -exec ls -l >> $TEMP.REMAIN {} \; > /dev/null 2>&1
		if [ -s $TEMP.REMAIN ] 
		then
       			echo \
       "Please review $TEMP.REMAIN for source files that should be under SCCS."
		else
	        	echo "No files remaining."
		fi	

#if object files remain after edit
	[ -s $TEMP.OBJECTS ]  && echo \
"	You can use $TEMP.OBJECTS as reference in fixing Makefile
	'clean' targets."
fi

if [ "$UNMOUNTED" ] 
then
	DO_MT=1 
	eval "$GET_MTSTAT" && exit 0 
else
	DO_MT=0 
	eval "$GET_MTSTAT"&& exit 0
fi
exit 1

