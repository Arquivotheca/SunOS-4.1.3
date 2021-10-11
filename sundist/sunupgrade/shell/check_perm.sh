#!/bin/sh
#
#       @(#)check_perm.sh	1.4 8/28/91 SMI
#
# check_perm.sh
#
# compare permissions, sizes, dates from what is in the 
# original release media and what is local
#
export TZ
TZ=US/Pacific
CMD=`basename $0`
USAGE="$0 [-verbose]"
MEDIA_DIR=/usr/etc/install/tar
UPGRADE_DIR=$MEDIA_DIR/sunupgrade
INCLUDE_DIR=$UPGRADE_DIR/incld
LS=$UPGRADE_DIR/toolkit/newls
TMP=/tmp
VERBOSE=""
KARCH=`arch -k`
KARCH1=sun4
KARCH2=sun4c
KARCH3=sun4m
INSTALL_DATE=""
INSDIR=/etc/install

if [ -r $INSDIR/release ]; then
    FROM_RELEASE=`sed "s/^.*sunos\.//" $INSDIR/release`
elif [ -r /usr/sys/conf.common/RELEASE ]; then
    FROM_RELEASE=`cat /usr/sys/conf.common/RELEASE`
else
    echo "OS Release Version not found. Exiting with code 0."
    exit 0
fi

if [ "$FROM_RELEASE" = "4.1.2" ]; then
	rev=".412"
elif [ "$FROM_RELEASE" = "4.1.1" ]; then
   if [ -f /usr/kvm/showrev.dat ]; then
	rev=`nawk '{print "."$NF}' /usr/kvm/showrev.dat |\
		tr '[A-Z]' '[a-z]'`
   fi
else
	echo "$FROM_RELEASE is not supported in $CMD. Exiting with code 2."
	exit 2
fi

FILE_DB=file_db$rev
ROOT_MEDIA_PATHS=$UPGRADE_DIR/$FILE_DB/root_media_paths
USR_MEDIA_PATHS=$UPGRADE_DIR/$FILE_DB/usr_media_paths
KVM_MEDIA_PATHS=$UPGRADE_DIR/$FILE_DB/kvm_media_paths.$KARCH
TMP_MEDIA_PATHS=$TMP/tmp_media_paths

ROOT_MEDIA_FILES_DATABASE=$UPGRADE_DIR/$FILE_DB/root_media_files_database
USR_MEDIA_FILES_DATABASE=$UPGRADE_DIR/$FILE_DB/usr_media_files_database
KVM_MEDIA_FILES_DATABASE=$UPGRADE_DIR/$FILE_DB/kvm_media_files_database.$KARCH
TMP_MEDIA_FILES_DATABASE=$TMP/tmp_media_files_database

COMMON_FILE_DATABASE=$TMP/common_file_database
LOCAL_PATH_LIST=$TMP/local_path_list
LOCAL_PATHS=$TMP/local_paths

VOLATILE_CANDIDATES=/usr/tmp/volatile_candidates
TMP_VOLATILE_FILE=$TMP/volatile_file
VOLATILE_FILE=/usr/tmp/volatile_file

########################################################################
#
#
# Utility subroutines
#
#
########################################################################
cleanup() {
    if [ $1 -ne 0 ]; then
	verbose_log "Exiting with code $1"
    else
	verbose_log "Exiting"
    fi
    rm -f $COMMON_FILE_DATABASE \
	$LOCAL_PATHS $TMP_MEDIA_PATHS \
	$TMP_MEDIA_FILES_DATABASE \
	$TMP_VOLATILE_FILE
    exit $1
}
########################################################################
verbose_log() {
    if [ $VERBOSE ]
    then
	now=`date | nawk '{print $4}'`
	echo $now $*
    fi
}
########################################################################
user_handle() {
    echo "User routine: current name $NAME"
    cleanup 0
}
########################################################################
get_local_paths() {
	sort -u $1 > $LOCAL_PATHS
	rm -f	$1
}
########################################################################
get_common_database() {
    rm -f $COMMON_FILE_DATABASE

    for NAME in $common_paths; do
#
# if it is a directory, we need the second line
#
    if [ -d $NAME ] 
    then
    	$LS -aln $NAME | \
	nawk '/ \.$/ { print $1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" '$NAME'"}' \
				 >> $COMMON_FILE_DATABASE
	elif [ -f $NAME ]; then
	    LOCAL_DIRECT_STRING=`$LS -ln $NAME`
	    echo $LOCAL_DIRECT_STRING >> $COMMON_FILE_DATABASE
	fi
    done
}
########################################################################
check_it(){
    local_paths=$1
    media_paths=$2
    media_files_database=$3
    orig_media_paths=$4
    get_local_paths $local_paths
    common_paths=`comm -12 $LOCAL_PATHS $media_paths`
    get_common_database
    do_compare $media_files_database $media_paths \
	 $COMMON_FILE_DATABASE $orig_media_paths
}
########################################################################
make_local_list(){
    base_dir=$1
    rm -f $LOCAL_PATH_LIST
    for pack in $2; do
	nawk '{ name="'$base_dir'" substr($1,2); print name }' $pack >>  \
					$LOCAL_PATH_LIST
    done
}
########################################################################
do_compare(){
DATABASE=$1
PATHS=$2
COMMON_LIST=$3
ORIG_PATHS=$4
nawk '
function bingetpath(upath) {
    low = 1;
    high = media_path_rec;
    mid = int((low + high) / 2);
    while(low != high) {
	if(upath == media_path[mid])
	    return(mid)
	else if(upath < media_path[mid])
	    high = mid;
	else
	    low = mid;
	mid = int((low + high) / 2);
    }
    return(0);
}
BEGIN { 
    install_date="'$INSTALL_DATE'";
    media_base_rec = 0;
    while (getline <"'$DATABASE'"  > 0) {
	media_base_rec++;
	media_base[media_base_rec] = $0;
    }

    media_path_rec = 0;
    while (getline <"'$PATHS'"  > 0) {
	media_path_rec++;
	media_path[media_path_rec] = $0;
    }

    common_path_rec = 0;
    while (getline <"'$COMMON_LIST'"  > 0) {
	common_path_rec++;
	common_path[common_path_rec] = $0;
    }

    if ("'$ORIG_PATHS'" != ""){
	orig_path_rec = 0;
	while (getline <"'$ORIG_PATHS'"  > 0) {
	    orig_path_rec++;
	    orig_path[orig_path_rec] = $0;
	}
    }
    for(i = 1; i <= common_path_rec; i++) {
	split(common_path[i], local_dir);
	upath = local_dir[9];
	rec_num = bingetpath(upath);
	if(rec_num == 0) print "not found " upath
	else {
	    if ("'$ORIG_PATHS'" == "")
		org_path=upath
	    else
		org_path=orig_path[rec_num]
	    split(media_base[rec_num], media_dir);
	    local_permissions=substr(local_dir[1], 2);
	    media_permissions=substr(media_dir[1], 2);
	    local_group=local_dir[4];
	    media_group=media_dir[3];
	    local_size=local_dir[5];
	    media_size=media_dir[4];
	    local_owner=local_dir[3];
	    media_owner=media_dir[2];
	    local_date=sprintf("%s:%s:%s", local_dir[6], local_dir[7], local_dir[8]);
	    media_date=sprintf("%s:%s:%s", media_dir[5], media_dir[6], media_dir[8]);
	    if (local_permissions != media_permissions){
		print upath "	# permissions: local -> " local_permissions " media -> " media_permissions
		print "+ " org_path >> "'$TMP_VOLATILE_FILE'"
	    }
	    if (local_owner != media_owner){
		print upath "	# owner: local -> " local_owner " media -> " media_owner
		print "+ " org_path >> "'$TMP_VOLATILE_FILE'"
	    }
	    if (local_group != media_group){
		print upath "	# group: local -> " local_group " media -> " media_group
		print "+ " org_path >> "'$TMP_VOLATILE_FILE'"
	    }
	    if (local_date != media_date && substr(media_dir[1],1,1) != "d") {
		if(install_date == "") {
		    if(local_date != install_date){
			print upath "	# date: local -> " local_date " media -> " media_date
			print "+ " org_path >> "'$TMP_VOLATILE_FILE'"
	    	    }
		}
		else {
		    print upath "	# date: local -> " local_date " media -> " media_date
		    print "+ " org_path >> "'$TMP_VOLATILE_FILE'"
	    	}
	    }

	    if (local_size != media_size) {
		if(media_size != 0){
		    print upath "	# size: local -> " local_size " media -> " media_size
		    print "+ " org_path >> "'$TMP_VOLATILE_FILE'"
	    	}
	    }
	}
    }
    exit;
}

' >> $VOLATILE_CANDIDATES
}
########################################################################
trap 'cleanup 1' 1
trap 'cleanup 2' 2
trap 'cleanup 3' 3
trap 'cleanup 15' 15
trap 'user_handle 30' 30
trap 'user_handle 31' 31

if [ -f $UPGRADE_DIR/release ]; then
    TO_RELEASE=`sed 's/\./_/g' $UPGRADE_DIR/release | tr '[A-Z]' '[a-z]`
else
    echo "$UPGRADE_DIR/release not found"
    cleanup 3
fi

FROM_SUFFIX=`echo $FROM_RELEASE | sed "s/\.//g" | tr '[A-Z]' '[a-z]'`
TO_SUFFIX=`echo $TO_RELEASE | sed "s/\.//g" | tr '[A-Z]' '[a-z]'`

echo "# Suggested volatile file list
#
# key:
#       - $Release file is retained; $TO_RELEASE file added with .$TO_SUFFIX tag.
#       + $TO_RELEASE file replaces $Release file; $Release file saved with .$FROM_SUFFIX tag.
#       & $Release file replaced by $TO_RELEASE file.
#"> $VOLATILE_FILE

########################################################################
#
# Parse the arguments
#
########################################################################
for param in $*; do
    case $param in
	-v*) VERBOSE=1 ;;

	-I*)	INSTALL_DATE=`expr $param : '-I\(.*\)' '|' ""` 
		verbose_log "Install date is $INSTALL_DATE " ;;

    	*)	echo
		echo USAGE : $USAGE
		cleanup 1 ;;
    esac
done
########################################################################
#
# check for file existance
#
########################################################################
for file in $ROOT_MEDIA_PATHS $USR_MEDIA_PATHS $KVM_MEDIA_PATHS \
	$ROOT_MEDIA_FILES_DATABASE $USR_MEDIA_FILES_DATABASE \
	$KVM_MEDIA_FILES_DATABASE; do
    if [ ! -f $file ]; then
	echo "Database $file is not available."
	cleanup 8
    fi
done
########################################################################
if [ $KARCH = $KARCH1 ]; then
    KARCH_ADD1=$KARCH2
    KARCH_ADD2=$KARCH3
elif [ $KARCH = $KARCH2 ]; then
    KARCH_ADD1=$KARCH1
    KARCH_ADD2=$KARCH3
elif [ $KARCH = $KARCH3 ]; then
    KARCH_ADD1=$KARCH1
    KARCH_ADD2=$KARCH2
else
    echo $KARCH is not supported
    cleanup 12
fi
########################################################################
if [ -r $INSDIR/sys_info ]; then
    TYPE=`nawk -F= '{ if ($1 == "sys_type") print $2}' \
                                $INSDIR/sys_info`
else
    echo "$INSDIR/sys_info not found"
    echo "sys_type=client assumed"
    TYPE=client
fi
########################################################################
#
# Look who is running
#
########################################################################
if [ $TYPE = server -a "`whoami`x" != "root"x ]; then
    echo "You must be root to run $CMD at the server!"
    cleanup 4
fi
########################################################################

rm -f $VOLATILE_CANDIDATES

    verbose_log Testing root ...
    root_pack=$INCLUDE_DIR/proto_root_sunos_$TO_RELEASE.include
    make_local_list "" $root_pack
    check_it $LOCAL_PATH_LIST \
	     $ROOT_MEDIA_PATHS \
	     $ROOT_MEDIA_FILES_DATABASE
#-----------------------------------------------------------------
if [ $TYPE != dataless -a $TYPE != client ]; then
    verbose_log Testing kvm ...
    kvm_packs="$INCLUDE_DIR/${KARCH}_sunos_$TO_RELEASE.kvm.include \
	       $INCLUDE_DIR/${KARCH}_sunos_$TO_RELEASE.sys.include"
    make_local_list "/usr/kvm" "$kvm_packs"
    check_it $LOCAL_PATH_LIST \
	     $KVM_MEDIA_PATHS \
	     $KVM_MEDIA_FILES_DATABASE
fi
#-----------------------------------------------------------------
if [ $TYPE != dataless -a $TYPE != client ]; then
    verbose_log Testing usr ...
    all_other_packs=`ls $INCLUDE_DIR/*.include |\
		 egrep -v "kvm|sys|root"`
    make_local_list "/usr" "$all_other_packs"
    check_it $LOCAL_PATH_LIST \
	     $USR_MEDIA_PATHS \
	     $USR_MEDIA_FILES_DATABASE
fi
#-----------------------------------------------------------------
if [ $TYPE = server ]; then
    for KARCH_ADD in $KARCH_ADD1 $KARCH_ADD2; do
        SOFT_INFO1=$INSDIR/soft_info.sun4.${KARCH_ADD}.sunos.4.1.1
        ADD_KVM_PATHS1=$UPGRADE_DIR/file_db/kvm_media_paths.$KARCH_ADD
        ADD_KVM_DB1=$UPGRADE_DIR/file_db/kvm_media_files_database.$KARCH_ADD
        SOFT_INFO2=$INSDIR/soft_info.sun4.${KARCH_ADD}.sunos.4.1.2
        ADD_KVM_PATHS2=$UPGRADE_DIR/file_db.412/kvm_media_paths.$KARCH_ADD
        ADD_KVM_DB2=$UPGRADE_DIR/file_db.412/kvm_media_files_database.$KARCH_ADD
        if [ -f $SOFT_INFO1 ]; then
	   verbose_log Testing kvm of the $KARCH_ADD on 4.1.1 ...
	   kvm_path=`nawk -F "=" '/kvm_path/ {print $2}' $SOFT_INFO1`
	   kvm_packs="$INCLUDE_DIR/${KARCH_ADD}_sunos_$TO_RELEASE.kvm.include \
	           $INCLUDE_DIR/${KARCH_ADD}_sunos_$TO_RELEASE.sys.include"
	   make_local_list $kvm_path "$kvm_packs"
	   sed "s@/usr/kvm@$kvm_path@" $ADD_KVM_PATHS1 > \
				$TMP_MEDIA_PATHS
	   sed "s@/usr/kvm@$kvm_path@" $ADD_KVM_DB1 > \
				$TMP_MEDIA_FILES_DATABASE
	   check_it	$LOCAL_PATH_LIST \
	     		$TMP_MEDIA_PATHS \
	     		$TMP_MEDIA_FILES_DATABASE \
			$ADD_KVM_PATHS1

	   rm -f $TMP_MEDIA_PATHS $TMP_MEDIA_FILES_DATABASE
       fi
       if [ -f $SOFT_INFO2 ]; then
	   verbose_log Testing kvm of the $KARCH_ADD on 4.1.2 ...
	   kvm_path=`nawk -F "=" '/kvm_path/ {print $2}' $SOFT_INFO2`
	   kvm_packs="$INCLUDE_DIR/${KARCH_ADD}_sunos_$TO_RELEASE.kvm.include \
	           $INCLUDE_DIR/${KARCH_ADD}_sunos_$TO_RELEASE.sys.include"
	   make_local_list $kvm_path "$kvm_packs"
	   sed "s@/usr/kvm@$kvm_path@" $ADD_KVM_PATHS2 > \
				$TMP_MEDIA_PATHS
	   sed "s@/usr/kvm@$kvm_path@" $ADD_KVM_DB2 > \
				$TMP_MEDIA_FILES_DATABASE
	   check_it	$LOCAL_PATH_LIST \
	     		$TMP_MEDIA_PATHS \
	     		$TMP_MEDIA_FILES_DATABASE \
			$ADD_KVM_PATHS2

	   rm -f $TMP_MEDIA_PATHS $TMP_MEDIA_FILES_DATABASE
       fi
     done
    #-----------------------------------------------------------------
    for karch in $KARCH1 $KARCH2 $KARCH3; do
	client_list_file1=$INSDIR/client_list.sun4.$karch.sunos.4.1.1
	if [ -f $client_list_file1 ]; then
	    client_list1="$client_list1 `cat $client_list_file1`"
	fi
	client_list_file2=$INSDIR/client_list.sun4.$karch.sunos.4.1.2
	if [ -f $client_list_file2 ]; then
	    client_list2="$client_list2 `cat $client_list_file2`"
	fi
    done
    for client1 in $client_list1; do
	client_file1=$INSDIR/client.$client1
	if [ $client_file1 ]; then
	    verbose_log Testing root of the client $client1 ...
	    root_path=`nawk -F "=" '/root_path/ {print $2}' $client_file1`
	    make_local_list $root_path "$root_pack"
	    sed "s@^@$root_path@" $UPGRADE_DIR/file_db/root_media_paths > \
				$TMP_MEDIA_PATHS
	    sed "s@/usr/kvm@$kvm_path@" $UPGRADE_DIR/file_db/root_media_files_database > \
				$TMP_MEDIA_FILES_DATABASE
	    check_it	$LOCAL_PATH_LIST \
	    		$TMP_MEDIA_PATHS \
	     		$TMP_MEDIA_FILES_DATABASE \
			$UPGRADE_DIR/file_db/root_media_paths

	    rm -f $TMP_MEDIA_PATHS $TMP_MEDIA_FILES_DATABASE
	else
	    echo File $INSDIR/client.$client1 not found
	fi
    done
    for client2 in $client_list2; do
	client_file2=$INSDIR/client.$client2
	if [ $client_file2 ]; then
	    verbose_log Testing root of the client $client2 ...
	    root_path=`nawk -F "=" '/root_path/ {print $2}' $client_file2`
	    make_local_list $root_path "$root_pack"
	    sed "s@^@$root_path@" $UPGRADE_DIR/file_db.412/root_media_paths > \
				$TMP_MEDIA_PATHS
	    sed "s@/usr/kvm@$kvm_path@" $UPGRADE_DIR/file_db.412/root_media_files_database > \
				$TMP_MEDIA_FILES_DATABASE
	    check_it	$LOCAL_PATH_LIST \
	    		$TMP_MEDIA_PATHS \
	     		$TMP_MEDIA_FILES_DATABASE \
			$UPGRADE_DIR/file_db.412/root_media_paths

	    rm -f $TMP_MEDIA_PATHS $TMP_MEDIA_FILES_DATABASE
	else
	    echo File $INSDIR/client.$client2 not found
	fi
    done
fi
#-----------------------------------------------------------------
[ -s $TMP_VOLATILE_FILE ] && \
sort -u $TMP_VOLATILE_FILE >> $VOLATILE_FILE
echo "The results are placed to the"
if [ -s $VOLATILE_CANDIDATES ]; then
    echo  $VOLATILE_CANDIDATES
    echo  and
fi
echo  $VOLATILE_FILE
cleanup 0
########################################################################
