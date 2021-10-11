#!/bin/sh
#
#       %W% %G% SMI
#
#set -x

# NAME :   compar_A_B
#
# Generate the list of the changes needed to upgrade
# sunos from release A to release B.
#
# Parameters:
#
# KARCH A_REL B_REL A_TAR B_TAR  DB_TMP DEST LOG PACK_LIST
#
# A_REL:	name of the A release (sunos_4_1_1)
# B_REL:	name of the B release (sunos_4_1_2)
#
# A_TAR:	path to the media directory of the A release (untared)
# B_TAR:	path to the media directory of the B release for sun4 or sun4c
#
# DB_TMP:	path to the temp directory.  
# 		This directory must have at least 26M space available.
#
# DEST:		directory for the results ({buildmachine}:/proto_sunupgrad/incld)
# LOG:		file for log output
# PACK_LIST:	a list of packages this script compares between 411 & 412
#

KARCH=$1
A_REL=$2
B_REL=$3
A_TAR=$4
B_TAR=$5
DB_TMP=$6
DEST=$7
LOG=$8
shift; shift; shift
shift; shift; shift
shift; shift
while [ "$#" -ne 0 ]; do
    PACK_LIST="$PACK_LIST $1"
    shift
done

HERE=`pwd`
EXPORT_EXEC=export/exec
AARCH=sun4
SIZE=$DEST/_size_diff

log_echo(){
    echo $*
    echo $* >> $LOG
}

log_cat(){
    cat $1
    cat $1 >> $LOG
}

cmp_by_names(){
# Check ls -lg first
    if [ -f $DA_TMP/$f -a -f $DB_TMP/$f ]; then
        ls -lg  $DA_TMP/$f $DB_TMP/$f|\
	nawk '{
    perm[NR]=$1
    owner[NR]=$3
    group[NR]=$4
    size[NR]=$5
    file[NR]=$NF
}
END{
    if (size[1] != size[2]){
	print "size"
	print perm[1], owner[1], group[1], size[1], file[1]
	print perm[2], owner[2], group[2], size[2], file[2] 
	exit 1
    } else if (perm[1] != perm[2]){
	print "perm"
	print perm[1], owner[1], group[1], size[1], file[1]
	print perm[2], owner[2], group[2], size[2], file[2] 
	print perm[1], owner[1], group[1], size[1], file[1]
	print perm[2], owner[2], group[2], size[2], file[2]
	exit 2
    } else
	exit 0
}'
    rc=$?
	if [ $rc -ne 0 ]; then
	    log_echo  "."$f
	    echo  "."$f >> $PATH_NAME.replace
	    return
	fi
    fi

# Directories ? Ignore
    if [ -d $DA_TMP/$f ]; then
	return
    fi
# Links ? Where directed to?
    if [ -h $DA_TMP/$f ]; then
	for la in `ls -l $DA_TMP/$f`; do
	    :
	done
	for lb in `ls -l $DB_TMP/$f`; do
	    :
	done
	if [ $la != $lb ] ; then
	    log_echo  "."$f"->"$lb
	        echo  "."$f >> $PATH_NAME.relink
	fi
	return
    fi
# Files ? cmp them !
    if [ -f $DA_TMP/$f ]; then
# Not readable ? Make readable
	[ ! -r $DA_TMP/$f ] && chmod +r $DA_TMP/$f
	[ ! -r $DB_TMP/$f ] && chmod +r $DB_TMP/$f
	cmp -s $DA_TMP/$f $DB_TMP/$f
	rc=$?
	case $rc 
	    in
	    0) ;;
	    1) 
		log_echo  "."$f
	        echo  "."$f >> $PATH_NAME.replace ;;
	    *)
	        log_echo "rc="${rc}" in ."$f
		log_echo "Check "$DA_TMP/$f" and "$DB_TMP/$f
		ls $DA_TMP/$f $DB_TMP/$f
		ls $DA_TMP/$f $DB_TMP/$f >> $LOG
	        echo  "."$f >> $PATH_NAME.replace ;;
	 esac
    fi
}
comp (){
    PATH_PACK_A=$1
    PATH_PACK_B=$2
    INC_NAME=$3
    PACK=$INC_NAME

    PATH_NAME=`echo $DEST/$INC_NAME | tr '[A-Z]' '[a-z]'`

    rm -f $PATH_NAME.*

    log_echo "PACKAGE "$PACK
    rm -rf $DB_TMP/* $DB_TMP/.[a-z]* $DB_TMP/.[A-Z]*
 
#    cd $DA_TMP
#    tar -xof $A_TAR/$PATH_PACK_A
    DA_TMP=$A_TAR/$PATH_PACK_A
    if [ -d $A_TAR/$PATH_PACK_A ]; then
	cd $A_TAR/$PATH_PACK_A
    else
	log_echo " $A_TAR/$PATH_PACK_A not found"
	exit 1
    fi

#
# Hack alert -- this script does not properly handle hard links. The 'du -a'
# command does not report hard links, but the 'find . -print' command
# does.  Use the 'find' logs to generate a package.new list, and use the
# 'du' logs to do a file by file comparison.  In this manner, new hard links
# are preserved (or at least the files are copied).  This should be revisited
# to find a proper solution.
#
    du -a | awk  ' {print $2 } ' | sort > /tmp/fl_$A_REL
    find . -print | sort > /tmp/hl_$A_REL
    sizea=`du -s . | (read s n; echo $s)`
 
    cd $DB_TMP
    if [ -f $PATH_PACK_B ]; then
	tar -xof $PATH_PACK_B
    else
	log_echo " $PATH_PACK_B not found"
	exit 2
    fi
    du -a | awk  ' {print $2 } ' | sort > /tmp/fl_$B_REL
    find . -print | sort > /tmp/hl_$B_REL 
    sizeb=`du -s . | (read s n; echo $s)`

    cd $HERE

    log_echo "Size "$A_REL" of "$PACK " is " $sizea
    log_echo "Size "$B_REL" of "$PACK " is " $sizeb
    dsize=`expr $sizeb - $sizea`
    log_echo "Diff_size " $PACK "	" $dsize
    if [ -f $SIZE ]; then
	grep -v "^$PACK" $SIZE > /tmp/$$
	cp /tmp/$$ $SIZE
        rm /tmp/$$
    fi
    echo $PACK "	" $dsize >> $SIZE

    log_echo "Only in "$A_REL" of "$PACK
    comm -23 /tmp/fl_$A_REL  /tmp/fl_$B_REL >\
				 $PATH_NAME.delete
    log_cat $PATH_NAME.delete

# Hack -- For the "additional" files, use the hard link file...
    log_echo "Only in "$B_REL" of "$PACK
    comm -13 /tmp/hl_$A_REL  /tmp/hl_$B_REL >\
				 $PATH_NAME.add
    rm -f /tmp/hl_$A_REL /tmp/hl_$B_REL
    log_cat $PATH_NAME.add

    comm -12 /tmp/fl_$A_REL  /tmp/fl_$B_REL  | \
    awk ' { fn=$1; fp=substr(fn,2); print fp} '> /tmp/comm
    fl=`cat /tmp/comm`

    log_echo "Different in "$A_REL" and "$B_REL" of "$PACK

    for f in $fl; do
	cmp_by_names
    done

    for name in add replace relink
    do
	if [ -f $PATH_NAME.$name ]
	then
	    cat $PATH_NAME.$name >>\
		$PATH_NAME.include
	    rm -f $PATH_NAME.$name
	fi
    done

# Clean up now
    rm -rf $DB_TMP/* $DB_TMP/.[a-z]* $DB_TMP/.[A-Z]*

    for name in add delete replace include relink
    do
	if [ -f $PATH_NAME.$name -a\
	   ! -s $PATH_NAME.$name ]
	then
	    rm -f $PATH_NAME.$name
	fi
    done
}

date > $LOG

for pckg in $PACK_LIST; do
    pckg_sm=`echo $pckg | tr '[A-Z]' '[a-z]'`
    case $pckg in

	Kvm)
	rm -f ${DEST}/${KARCH}_*.kvm.*
	comp	$EXPORT_EXEC/kvm/${KARCH}_$A_REL/kvm\
		$B_TAR/${KARCH}/Kvm\
		${KARCH}_$B_REL.kvm;;

	Sys)
        rm -f ${DEST}/${KARCH}_*.sys.*
	comp	$EXPORT_EXEC/kvm/${KARCH}_$A_REL/sys\
		$B_TAR/${KARCH}/Sys\
		${KARCH}_$B_REL.sys;;

	root)
        rm -f ${DEST}/*root*
	comp	$EXPORT_EXEC/proto_root_$A_REL\
		$B_TAR/${KARCH}/root\
		proto_root_$B_REL;;

	Manual)
	comp	export/share/$A_REL/manual\
		$B_TAR/${KARCH}/Manual\
		manual;;

	*)
        comp    $EXPORT_EXEC/${AARCH}_$A_REL/$pckg_sm\
		$B_TAR/${KARCH}/$pckg\
		$pckg_sm;;
    esac
done

date >> $LOG
