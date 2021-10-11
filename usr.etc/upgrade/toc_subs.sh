#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# NAME: toc_subs
#
# These routines read from the toc file on disk.


#
# read the toc and return the media volume, arch, and release number
#
# output:
#       VOLUME  -- which volume this media is
#       ARCH    -- arch of this media
#       RELEASE -- which release this media is
#
toc_release_info() {
    eval ` awk '
        $1 == "VOLUME" { printf( "VOLUME=%s ", $2 ) }
        $1 == "ARCH"   { printf( "ARCH=%s ", $2 ) }
        $1 == "SunOS"  { printf( "RELEASE=%s ", $2 ) } ' ${TOCFILE}`
}
 
#
# return the volume, file number, and file type for this package
#
# output: 
#   U_VOLUME -- volume that the upgrade is on
#   U_FILE   -- the file on that volume that upgrade is on
#   U_TYPE   -- file type (tar or tarZ)
 
toc_tar_info() {
    eval ` awk '
        $3 == "'${PACKAGE}'" {
        printf( "U_VOLUME=%s U_FILE=%s U_TYPE=%s ",$1,$2,$5 ); } ' ${TOCFILE} `
}

#
# return list of tar packages in the toc
#
# output:    package_list -- list of tar packages
 
toc_tar_list() {
    package_list=""
    eval ` awk ' $5 ~ /tar/ || $5 ~ /tarZ/ { list = list " " $3 }
    END { printf ( "package_list=\"%s\" ", list ) }
    ' ${TOCFILE} `
}

# get list of packages to be upgraded by looking in the media
# info file to see what is loaded, and checking for a matching
# .include file.
# assume required packages will be upgraded

get_package_list () {
 
        toc_tar_list 
  
        if [ ! "$package_list" ]; then 
            echo "$CMD: Unable to extract package list from table of contents."             cleanup 1 
        fi 
 
        UPGRADE_LIST=""
 
        for PACKAGE in $package_list 
        do 
            echo $REQUIRED | grep -s $PACKAGE && [ -f ${PACKAGE}.${SUFFIX} ] \
                && UPGRADE_LIST="$UPGRADE_LIST $PACKAGE" && continue 
  
            # check the media file for other packages 
            line=`grep -n "mf_name=$PACKAGE" $MEDIA_FILE |
                   awk -F: '{ printf $1 }'`
            loaded=`tail +"${line}" $MEDIA_FILE | grep mf_loaded |
                   head -1 | sed "s/.*mf_loaded=//`
            if [ "$loaded" = "yes" -a  -s ${PACKAGE}.${SUFFIX} ]; then
                   UPGRADE_LIST="$UPGRADE_LIST $PACKAGE"
            fi
        done
}
 

