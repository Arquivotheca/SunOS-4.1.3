#! /bin/sh
#
## %Z%%M% %I% %E% Copyright 1990 Sun Microsystems, Inc.
##########################################################################
#  
# This utility archives the files required to build the cdrom release for
# 4.1 and 4.1PSR_A.  It mounts up each build machine for the specified
# release and tars them onto a diskfarm partition.  Archived copies
# are compared against original files using 'du -s' for patches and
# ls -l for sundist and tarfiles.
# 
# The archives are layed out according to a structure expected by the
# cdrom makefile which generates the final CD image on the CDROM 
# build machine (~sundist/cdimage.mk).
# 
# REQUIREMENTS:
# 
# Partition size:
# 
#    We use different partitions for 4.1 and 4.1PSR_A.  It is a good
#    idea to have two partitions available for each release to allow you 
#    to rotate them between release cycles.  Space requirements are:
# 
# 	4.1       (archs: sun3,sun3x,sun4,sun4c)	~ 380 MB
# 	4.1_PSR_A (arch : sun4)				~ 100 MB
# 
# Partition name:
# 
#    Be sure to prepare the partition and mount it rw on a directory
#    named /usr/cdrom_$(RELEASE_NAME).  RELEASE_NAME should usually be
#    identical to build_machine:/proto/usr/sys/conf.common/RELEASE, but 
#    if you have more than one run for a given release, you have 
#    the option of identifying your archives by another name.
#
#    Caveat:  if RELEASE_NAME contains dashes, you have to rename your
#    partition by converting dashes to underbars.  This script 
#    always expect your partition name to have been converted.	
#    (Refer to comments under release_name, below)
# 
#    Remember, that the RELEASE_NAME you choose should match the 
#    release named on your CDROM build machine in /usr/src/sundist/RELEASE 
#    (or PSR_RELEASE).  Those release files specify which archives will be
#    accessed to generate the CD image.
#
# Diskfarm name;
#
#    The diskfarm for 4.1 and 4.1PSR_A is 'valis'.  If you should ever
#    use another host to archive the cdrom files, make sure to update
#    the 'HOST' macro in ~sundist/cdimage.mk.
# 
# USAGE:
# 
# Invoke the script and specify the OS release category to be archived:
# 
# 		save_cd_depends 4.1 [-i]
# 		      or
# 		save_cd_depends psr [-i]
# 
# By default, ALL files upon which the cdrom release depends will be
# archived for each architecture.  This may take as long as 7 hours
# for the full 4.1 set, when build machines are located in another building.
# 
# If you wish to archive only certain sets of files from the build machines, 
# invoke the script using the interactive (-i) option. 
# 
# Upon completion, be sure to do the following:
# 
# 	o	Check the logfile for errors
# 	o	Remount the archive partition read-only
# 	o	Update /etc/exports and execute 'exportfs'
# 
# Please send bugs/RFE's to Janet Timbol (janet@firenze)
# 
###########################################################################

usage="USAGE:  $0 base|psr [-i] "
logfile=/usr/tmp/logfile_$$

[ ! `whoami` = root ] && echo "Not superuser!" && exit 1

if [ $# = 0 ] ; then 
	bad=1
else
	for i in $*; do
		case $i in
		      base)	release=base;; 
		[Pp][Ss][Rr] ) 	release=psr;;
			  -i ) 	interactive=1;;
			   * )	bad=1;;
		esac
	done
fi

[ "$bad" ] && echo $usage && exit 1

get_yn='(while read yn; do
	case "$yn" in
		[Yy]* ) exit 0;;
		[Nn]* ) exit 1;;
		    * ) echo -n "Please answer y or n:  ";;
	esac
done)'

#
# Mount and unmount partitions
#

get_mtstat='( [ "$do_mt" = "0" ] && cmd="/etc/mount" || cmd="/etc/umount"
done=""
while :
do
	/etc/mount|grep -s "$archive_mt on $mt_dir "
	if [ "$?" = "$do_mt" ]; then
		exit 0
	fi
	if [ "$done" ]; then
		echo -n "Unable to $cmd $mt_dir! " | tee -a $logfile
		echo "" | tee -a $logfile
		exit 1
	fi
	if [ "$do_mt" = 0 ]; then
		echo "Mounting ${archive_path} to $mt_dir" | tee -a $logfile
		${cmd} -o ro  ${archive_path} $mt_dir  >> $logfile 2>&1
	else
		echo "Unmounting ${archive_path}
		" | tee -a $logfile
		${cmd} $mt_dir >> $logfile 2>&1
	fi
	done=1
done )'

#
# Most release-specific vars are defined here
#

file_sets="patches tarfiles sundist"
arch_sets="sun4c sun4m sun4"
#arch_psr=sun4
#arch_psr=sun4c
arch_psr=sun4m
tar_dir=/tarfiles
sundist_dir=/usr/src/sundist
patch_dir=/proto/UNDISTRIBUTED/patches 

if [ "$release" = "base" ] ; then
	release_host=colander
#	tar_path_sun3x=iridium:${tar_dir}
#	sundist_path_sun3x=iridium:${sundist_dir}
	tar_path_sun4m=godzilla:${tar_dir}
	sundist_path_sun4m=godzilla:${sundist_dir}
	tar_path_sun4c=colander:${tar_dir}
	sundist_path_sun4c=colander:${sundist_dir}
	tar_path_sun4=krypton:${tar_dir}
	sundist_path_sun4=krypton:${sundist_dir}
#	tar_path_sun3=radium:${tar_dir}
#	sundist_path_sun3=radium:${sundist_dir}
	patch_path=${release_host}:${patch_dir}
else
#	release_host=locii; tar_dir=/tarfiles.combined
#	tar_path_sun4=${release_host}:${tar_dir}
#	sundist_path_sun4=${release_host}:${sundist_dir}
#	patch_path=${release_host}:${patch_dir}
	release_host=colander; tar_dir=/tarfiles
	tar_path_sun4m=${release_host}:${tar_dir}
	sundist_path_sun4m=${release_host}:${sundist_dir}
	tar_path_sun4c=${release_host}:${tar_dir}
	sundist_path_sun4c=${release_host}:${sundist_dir}
	patch_path=${release_host}:${patch_dir}
fi

#
# Set up archive partition
#
# We convert dashes to underbar as a work-around, since HSFS does
# not recognize it.  Other restrictions are automatically transformed
# by the '~suninstall/cd_tar' program.

echo "Determining release name from $release_host..."
release_name=`rsh $release_host cat /proto/usr/sys/conf.common/RELEASE| \
sed 's/-/_/g'`

echo -n "Use ${release_name:=UNKNOWN} to identify these archives?  "
eval "$get_yn"
if [ $? = 1 ]; then
	echo -n "Enter release name to be used:  "
	read release_conv
	release_name=`echo $release_conv | sed 's/-/_/g'`
fi


cd_archive=/usr/cdrom_${release_name}
build_mount=/mnt/cdbuild

if /etc/mount | egrep " on /mnt "; then
	echo "ERROR:  Unmount filesystem on /mnt! " | tee -a $logfile
	exit 1
else
	[ ! -d $build_mount ] && mkdir $build_mount
fi

/etc/mount | egrep " $cd_archive " > /tmp/$$

if [ -s /tmp/$$ ] ; then
	echo ""
	df `awk '{print $1 }' /tmp/$$`
	echo ""
	echo -n "Ok to use this partition? (y/n)  "
	eval "$get_yn"
	if [ "$?" = 1 ]; then
		echo "Please mount '$cd_archive' on another partition."
		exit 1
	fi
	perm=`awk '{print $NF}' /tmp/$$`
	if [ "$perm" = "(ro)" ]; then
		echo "Please remount '$cd_archive' for writing."
		exit 1
	fi
else
	echo "Partition '$cd_archive' not mounted."
	echo "Please set up the partition and mount it read-write."
	exit 1
fi

#
# Determine file sets to be archived; default is all
#

if [ "$interactive" ] ; then
	echo ""
	echo "Defining file sets to be archived..."
	for i in $file_sets; do
		echo -n "	Archive $i?	(y/n)  "
		eval "$get_yn" || continue
		case $i in
			patches ) get_patches=1;;
			tarfiles) get_tarfiles=1;;
			sundist ) get_sundist=1;;
		esac
		do_sets="$do_sets $i"
	done
else
	get_patches=1; get_tarfiles=1; get_sundist=1
	do_sets="$file_sets"
fi

[ ! "$do_sets" ] && echo "No files to archive!  Exiting..."  && exit 1

#
# Determine architectures to be archived; default is all
#

if [ "$release" = "base" ] ; then
	if [ "$interactive" ] ; then
		echo "Defining architectures to be archived..."
		for i in $arch_sets; do
			echo -n "	Archive $i?	(y/n)  "
			eval "$get_yn" || continue
			do_archs="$do_archs $i"
		done
	else
		do_archs=$arch_sets
	fi
else
	do_archs=${arch_psr}
fi

[ ! "$do_archs" ] && echo "No archs chosen!  Exiting..."  && exit 1

#
# Get confirmation before proceeding
#

echo "Archiving ${release_name} on `date`
" > $logfile
echo "
FILE SETS to be archived: $do_sets" | tee -a $logfile
echo "ARCHS to be archived:  $do_archs
" | tee -a $logfile
echo ""
echo -n "OK to proceed? (y/n)  "
if eval "$get_yn"; then
	:
else
	rm -f /tmp/$$ $logfile
	exit 1
fi

#
# Set up mount points
#

for i in `echo $do_sets`; do
	[ ! -d $cd_archive/${i} ] && \
	mkdir $cd_archive/${i}
	[ ! -d $build_mount/${i} ] && mkdir $build_mount/${i}
	[ "$i" = "patches" ] && continue
	for j in $do_archs; do
		[ ! -d $cd_archive/${i}/${j} ] && \
		mkdir $cd_archive/${i}/${j}
	done
done

trap 'echo ABORTED!|tee -a $logfile; cd $PWD; rm -f /tmp/*$$; exit 1' 1 2 3 15

#
# Archive the patches
#

if [ "$get_patches" ]; then
   while : ; do
	echo "Processing patches on `date`
	"  | tee -a $logfile
	do_mt=0; archive_path=${patch_path}; mt_dir=$build_mount/patches
	if eval "$get_mtstat"; then
		echo "Removing old ${cd_archive}/patches/*" | tee -a $logfile
		rm -rf ${cd_archive}/patches/*
		cd $build_mount/patches
		tar cfv - * | \
		(cd ${cd_archive}/patches; tar xfv - ) >> $logfile 2>&1
		du_src=`du -s $build_mount/patches| awk '{print $1}'`
		du_save=`du -s ${cd_archive}/patches| awk '{print $1}'`
		if [ ! "$du_src" = "$du_save" ] ; then
			echo "ERROR:  Mismatched size (build vs. archives)" | \
			tee -a $logfile
		fi
		echo "Patches archived on `date`" | tee -a $logfile
		cd $PWD; do_mt=1
		eval "$get_mtstat"
	else
		echo "ERROR:  Unable to archive patches!! " | tee -a $logfile
		echo "" | tee -a $logfile
	fi
	break
   done	
fi

#
# Archive the tarfiles
#

if [ "$get_tarfiles" ]; then
   echo "Processing tarfiles...
   " | tee -a $logfile
   for i in $do_archs; do
	echo "Start archiving $i tarfiles on `date`" | tee -a $logfile
	tar_arch=`eval echo \\$tar_path_$i`
	do_mt=0; archive_path=${tar_arch}; mt_dir=$build_mount/tarfiles
	if eval "$get_mtstat"; then
		echo "Removing old ${cd_archive}/tarfiles/${i}/*" |\
		tee -a $logfile
		rm -rf ${cd_archive}/tarfiles/${i}/*
		cd $build_mount/tarfiles
		tar cfv - ${i} | \
		(cd ${cd_archive}/tarfiles; tar xfv - )	>> $logfile 2>&1
		cd $i
		for j in `ls`; do
		   size_src=`ls -l ${j} | awk '{print $4}'`
		   size_save=`ls -l ${cd_archive}/tarfiles/${i}/${j} |\
		   awk '{print $4}'`
		   if [ ! "$size_src" = "$size_save" ] ; then
		     echo "ERROR: $j mismatched size (build vs. archives)"|\
		     tee -a $logfile
   		   fi
		done
		echo "$i tarfiles archived on `date`" | tee -a $logfile
		cd $PWD; do_mt=1
		eval "$get_mtstat"
	else
		echo "ERROR:  Unable to archive $i tarfiles!!" | tee -a $logfile
		echo "" | tee -a $logfile
	fi
   done
fi

#
# Archive the sundist images.
# We don't really need all the files defined in $sundist_files, but
# we're saving them, just in case...
#

if [ "$get_sundist" ]; then
   echo "Processing sundist files... 
   " | tee -a $logfile
   for i in $do_archs; do
   	sundist_files="miniroot.${i} cdrom_part_${i} ${i}_cdrom.xdr \
	bootimage.cdrom.${i} munix.cdrom.${i} munixfs.${i}.cdrom "
	echo "Start archiving $i sundist on `date`" | tee -a $logfile
	sundist_arch=`eval echo \\$sundist_path_$i`
	[ ! -d $build_mount/sundist/${i} ] && \
	mkdir $build_mount/sundist/${i}
	do_mt=0; archive_path=${sundist_arch} 
	mt_dir=$build_mount/sundist/${i}
	if eval "$get_mtstat"; then
	   echo "Removing old ${cd_archive}/sundist/${i}/*" |\
	   tee -a $logfile
	   rm -rf ${cd_archive}/sundist/${i}/*
	   cd $build_mount/sundist/${i}
	   tar cfv - `echo $sundist_files` | \
	   (cd ${cd_archive}/sundist/${i}; tar xfv - ) >> $logfile 2>&1
	   for j in $sundist_files; do
		   size_src=`ls -l ${j} | awk '{print $4}'`
		   size_save=`ls -l ${cd_archive}/sundist/${i}/${j} |\
		   awk '{print $4}'`
		   if [ ! "$size_src" = "$size_save" ] ; then
		     echo "ERROR: $j mismatched size (build vs. archives)"|\
		     tee -a $logfile
   		   fi
	   done
	   echo "$i sundist archived on `date`" | tee -a $logfile
	   cd $PWD; do_mt=1
	   eval "$get_mtstat"
	else
	   echo "ERROR:  Unable to archive $i tarfiles!!" | tee -a $logfile
	   echo "" | tee -a $logfile
	fi
   done
fi

for i in 'error ' 'eof' 'denied'; do
	if egrep -is "$i" $logfile ; then
	   echo "WARNING: '$i' found in $logfile!"
	   errs=1
	fi
done

if [ ! "$errs" ] ; then
	   echo "Archive completed on `date`" | tee -a $logfile
	   echo "Logfile is $logfile" | tee -a $logfile
	   echo ""
	   echo "Please remount archives read only, change fstab and exports!"
fi

cd $PWD
rm -f /tmp/*$$
